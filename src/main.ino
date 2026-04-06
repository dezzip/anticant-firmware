/*
 * main.ino — AntiCant Shooting Assistant Firmware
 * =================================================
 * Target:  Seeed Studio XIAO ESP32S3
 * Author:  AntiCant Project
 * Version: 1.0.0
 *
 * Main loop orchestrates all modules:
 *   IMU → Shot Detector → LEDs → E-Ink → BLE → Battery
 *
 * Button controls:
 *   Short press  → Cycle distance (100/300/600/1000m)
 *   Long press   → Cycle mode (TRAINING/COMPETITION/DISCRET)
 *   Double click  → Reset shot counter
 */

#include "config.h"
#include "imu.h"
#include "leds.h"
#include "eink.h"
#include "ble_service.h"
#include "battery.h"
#include "shot_detector.h"

// ============================================================================
// Global State
// ============================================================================
static DistancePreset currentDistance = DIST_100M;
static UserMode       currentMode    = MODE_TRAINING;
static bool           imuAvailable   = false;
static uint32_t       lastImuUs      = 0;

// ============================================================================
// Button State Machine
// ============================================================================
enum ButtonEvent {
    BTN_NONE,
    BTN_SHORT_PRESS,
    BTN_LONG_PRESS,
    BTN_DOUBLE_CLICK
};

static bool     btnLastState       = HIGH;  // INPUT_PULLUP → idle HIGH
static bool     btnState           = HIGH;
static uint32_t btnDebounceMs      = 0;
static uint32_t btnPressStartMs    = 0;
static bool     btnPressed         = false;
static bool     btnLongFired       = false;
static uint32_t btnLastReleaseMs   = 0;
static uint8_t  btnClickCount      = 0;
static bool     btnWaitDoubleClick = false;

static ButtonEvent buttonUpdate() {
    ButtonEvent event = BTN_NONE;
    bool reading = digitalRead(PIN_BUTTON);
    uint32_t now = millis();

    // Debounce
    if (reading != btnLastState) {
        btnDebounceMs = now;
    }
    btnLastState = reading;

    if ((now - btnDebounceMs) < BTN_DEBOUNCE_MS) {
        return event;
    }

    // State change detected
    if (reading != btnState) {
        btnState = reading;

        if (btnState == LOW) {
            // Button pressed
            btnPressed      = true;
            btnLongFired    = false;
            btnPressStartMs = now;
        } else {
            // Button released
            if (btnPressed && !btnLongFired) {
                // Short press completed
                btnClickCount++;
                btnLastReleaseMs   = now;
                btnWaitDoubleClick = true;
            }
            btnPressed = false;
        }
    }

    // Long press detection (while still held)
    if (btnPressed && !btnLongFired &&
        (now - btnPressStartMs >= BTN_LONG_PRESS_MS)) {
        btnLongFired    = true;
        btnClickCount   = 0;
        btnWaitDoubleClick = false;
        event = BTN_LONG_PRESS;
    }

    // Double click detection (after release, wait for second click or timeout)
    if (btnWaitDoubleClick && !btnPressed &&
        (now - btnLastReleaseMs >= BTN_DOUBLE_CLICK_MS)) {

        btnWaitDoubleClick = false;
        if (btnClickCount >= 2) {
            event = BTN_DOUBLE_CLICK;
        } else if (btnClickCount == 1) {
            event = BTN_SHORT_PRESS;
        }
        btnClickCount = 0;
    }

    return event;
}

// ============================================================================
// Handle button events
// ============================================================================
static void handleButton(ButtonEvent evt) {
    switch (evt) {
        case BTN_SHORT_PRESS:
            // Cycle distance: 100 → 300 → 600 → 1000 → 100 ...
            currentDistance = (DistancePreset)((currentDistance + 1) % DIST_COUNT);
            Serial.printf("[BTN] Distance → %d m (±%.1f°)\n",
                          DISTANCE_VALUES[currentDistance],
                          TOLERANCE_VALUES[currentDistance]);
            // Force e-ink refresh on distance change
            einkUpdate(imuGetCantAngle(), currentDistance, currentMode,
                       shotGetCount(), batteryGetPercent(), true);
            break;

        case BTN_LONG_PRESS:
            // Cycle mode: TRAINING → COMPETITION → DISCRET → TRAINING ...
            currentMode = (UserMode)((currentMode + 1) % MODE_COUNT);
            Serial.printf("[BTN] Mode → %s\n", MODE_NAMES[currentMode]);
            // Force e-ink refresh on mode change
            einkUpdate(imuGetCantAngle(), currentDistance, currentMode,
                       shotGetCount(), batteryGetPercent(), true);
            break;

        case BTN_DOUBLE_CLICK:
            // Reset shot counter
            shotReset();
            Serial.println("[BTN] Shot counter reset");
            einkUpdate(imuGetCantAngle(), currentDistance, currentMode,
                       shotGetCount(), batteryGetPercent(), true);
            break;

        default:
            break;
    }
}

// ============================================================================
// SETUP
// ============================================================================
void setup() {
    Serial.begin(115200);
    delay(500);  // Let USB serial connect
    Serial.println("=========================================");
    Serial.println("  AntiCant Shooting Assistant v1.0");
    Serial.println("  Seeed Studio XIAO ESP32S3");
    Serial.println("=========================================");

    // Button pin
    pinMode(PIN_BUTTON, INPUT_PULLUP);

    // Initialize all modules
    Serial.println("[INIT] Battery...");
    batteryInit();

    Serial.println("[INIT] IMU...");
    bool imuOk = imuInit();
    if (!imuOk) {
        Serial.println("[INIT] WARNING: IMU not found — running without IMU");
        Serial.println("[INIT] Cant angle fixed at 0.0 deg (always LEVEL)");
    }

    Serial.println("[INIT] LEDs...");
    ledsInit();

    Serial.println("[INIT] E-Ink display...");
    einkInit();

    Serial.println("[INIT] Shot detector...");
    shotInit();

    Serial.println("[INIT] BLE...");
    bleInit();

    imuAvailable = imuOk;
    Serial.println("[INIT] ===== ALL SYSTEMS GO =====");
    Serial.printf("[INIT] Distance: %d m | Mode: %s\n",
                  DISTANCE_VALUES[currentDistance],
                  MODE_NAMES[currentMode]);
}

// ============================================================================
// MAIN LOOP
// ============================================================================

void loop() {
    uint32_t nowUs = micros();
    uint32_t nowMs = millis();

    // ---- IMU Update @ 60 Hz ----
    if (imuAvailable && (nowUs - lastImuUs >= IMU_UPDATE_INTERVAL_US)) {
        lastImuUs = nowUs;

        // Read & filter IMU
        imuUpdate();

        // Shot detection on every IMU cycle
        shotUpdate(imuGetAccelMagnitude());
    }

    // ---- Button handling (every loop iteration) ----
    ButtonEvent btnEvt = buttonUpdate();
    if (btnEvt != BTN_NONE) {
        handleButton(btnEvt);
    }

    // ---- Cant angle: BLE override > IMU > 0° ----
    float cantAngle;
    if (bleHasOverride()) {
        // iPhone is controlling the angle
        cantAngle = bleGetOverrideAngle();
    } else if (imuAvailable) {
        cantAngle = imuGetCantAngle();
    } else {
        // No IMU, no override — fixed at 0°
        cantAngle = 0.0f;
    }

    // ---- Debug print every 500ms ----
    static uint32_t lastDebugMs = 0;
    if (nowMs - lastDebugMs >= 500) {
        lastDebugMs = nowMs;
        Serial.printf("[LOOP] cant=%.1f° tol=%.1f° mode=%s dist=%dm\n",
                      cantAngle, TOLERANCE_VALUES[currentDistance],
                      MODE_NAMES[currentMode],
                      DISTANCE_VALUES[currentDistance]);
    }

    // ---- LED update (every loop iteration for smooth animation) ----
    float tolerance = TOLERANCE_VALUES[currentDistance];
    ledsUpdate(cantAngle, tolerance, currentMode, batteryIsLow());

    // ---- Battery update (respects internal 10s interval) ----
    batteryUpdate();

    // ---- E-Ink update (respects internal 5s interval) ----
    einkUpdate(cantAngle, currentDistance, currentMode,
               shotGetCount(), batteryGetPercent());

    // ---- BLE update (respects internal 500ms interval) ----
    bleUpdate(cantAngle, currentMode, currentDistance,
              shotGetCount(), batteryGetPercent());
}
