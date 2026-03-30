/*
 * config.h — Configuration globale AntiCant Firmware
 * Toutes les constantes, pins, seuils et paramètres en un seul endroit.
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ============================================================================
// PINS
// ============================================================================
#define PIN_LEDS        D0      // WS2812B data (GPIO1)
#define PIN_BUTTON      D1      // Mode/distance button (INPUT_PULLUP, GPIO2)
#define PIN_BATTERY     D2      // Battery voltage divider (A2/GPIO3)

// I2C pour LSM6DS3 (SDA=D4/GPIO5, SCL=D5/GPIO6 — défaut Wire)
// SPI pour e-ink SSD1680 (SCK=D8/GPIO7, MOSI=D10/GPIO9 — défaut SPI)
#define PIN_EINK_CS     D7      // GPIO44
#define PIN_EINK_DC     D3      // GPIO4
#define PIN_EINK_RST    D6      // GPIO43
// BUSY non connecté — le driver utilise des délais (pas assez de GPIO)

// ============================================================================
// LEDs
// ============================================================================
#define NUM_LEDS        6
#define LED_BRIGHTNESS_DISCRET  51   // 20%
#define LED_BRIGHTNESS_NORMAL   140  // 55%
#define LED_BRIGHTNESS_MAX      255  // 100%

// ============================================================================
// IMU
// ============================================================================
#define IMU_UPDATE_HZ       60
#define IMU_UPDATE_INTERVAL_US  (1000000UL / IMU_UPDATE_HZ)  // ~16667 µs

// Kalman filter parameters
#define KALMAN_Q_ANGLE      0.001f   // Process noise — angle
#define KALMAN_Q_BIAS       0.003f   // Process noise — bias
#define KALMAN_R_MEASURE    0.03f    // Measurement noise

// ============================================================================
// SHOT DETECTION
// ============================================================================
#define SHOT_ACCEL_THRESHOLD    4.0f    // g — threshold for shot detection
#define SHOT_COOLDOWN_MS        300     // Minimum ms between two shots
#define SHOT_DOUBLE_CLICK_MS    400     // Max interval for double-click reset

// ============================================================================
// DISTANCES & TOLERANCES (degrees)
// ============================================================================
enum DistancePreset : uint8_t {
    DIST_100M  = 0,
    DIST_300M  = 1,
    DIST_600M  = 2,
    DIST_1000M = 3,
    DIST_COUNT = 4
};

static const uint16_t DISTANCE_VALUES[DIST_COUNT] = {
    100, 300, 600, 1000
};

static const float TOLERANCE_VALUES[DIST_COUNT] = {
    1.5f,   // 100m  → ±1.5°
    0.8f,   // 300m  → ±0.8°
    0.4f,   // 600m  → ±0.4°
    0.2f    // 1000m → ±0.2°
};

// Zone "approche" = tolérance × multiplier
#define APPROACH_ZONE_MULT  2.5f

// ============================================================================
// USER MODES
// ============================================================================
enum UserMode : uint8_t {
    MODE_TRAINING    = 0,
    MODE_COMPETITION = 1,
    MODE_DISCRET     = 2,
    MODE_COUNT       = 3
};

static const char* MODE_NAMES[MODE_COUNT] = {
    "TRAINING",
    "COMPET.",
    "DISCRET"
};

// ============================================================================
// BUTTON TIMING
// ============================================================================
#define BTN_DEBOUNCE_MS         50
#define BTN_LONG_PRESS_MS       2000    // >2s = long press (mode change)
#define BTN_DOUBLE_CLICK_MS     400     // Max gap for double click

// ============================================================================
// E-INK DISPLAY
// ============================================================================
#define EINK_REFRESH_INTERVAL_MS    5000    // Refresh every 5 seconds
#define EINK_WIDTH                  250
#define EINK_HEIGHT                 122

// ============================================================================
// BLE
// ============================================================================
#define BLE_DEVICE_NAME     "AntiCant"
#define BLE_NOTIFY_INTERVAL_MS  500

// Custom service & characteristic UUIDs
#define BLE_SERVICE_UUID            "12345678-1234-5678-1234-56789abcdef0"
#define BLE_CHAR_CANT_UUID          "12345678-1234-5678-1234-56789abcdef1"
#define BLE_CHAR_MODE_UUID          "12345678-1234-5678-1234-56789abcdef2"
#define BLE_CHAR_DISTANCE_UUID      "12345678-1234-5678-1234-56789abcdef3"
#define BLE_CHAR_SHOTS_UUID         "12345678-1234-5678-1234-56789abcdef4"
#define BLE_CHAR_BATTERY_UUID       "12345678-1234-5678-1234-56789abcdef5"

// ============================================================================
// BATTERY
// ============================================================================
#define BATTERY_READ_INTERVAL_MS    10000   // Read every 10 seconds
#define BATTERY_LOW_THRESHOLD       15      // % — low battery warning
#define BATTERY_VREF                3.3f
#define BATTERY_DIVIDER_RATIO       2.0f    // Voltage divider ratio
#define BATTERY_V_MIN               3.0f    // Empty LiPo
#define BATTERY_V_MAX               4.2f    // Full LiPo

// ============================================================================
// GREEN PULSE (BREATHING)
// ============================================================================
#define GREEN_HOLD_THRESHOLD_MS     1000    // 1s in zone → start breathing
#define BREATH_PERIOD_MS            2000    // Full breath cycle period

#endif // CONFIG_H
