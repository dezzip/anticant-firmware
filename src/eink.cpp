/*
 * eink.cpp — E-Ink Display Module implementation
 * SSD1680 2.13" via SPI, partial refresh every 5 seconds.
 */

#include "eink.h"
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_EPD.h>

// ============================================================================
// Display instance
// ============================================================================
static Adafruit_SSD1680 display(
    EINK_WIDTH, EINK_HEIGHT,
    PIN_EINK_DC, PIN_EINK_RST, PIN_EINK_CS,
    -1,   // SRCS — pas de SRAM externe
    -1    // BUSY non connecté — délai interne utilisé
);

static uint32_t lastRefreshMs = 0;
static bool     initialized   = false;

// ============================================================================
// Helper: draw a horizontal line divider
// ============================================================================
static void drawDivider(int16_t y) {
    display.drawFastHLine(0, y, EINK_WIDTH, EPD_BLACK);
}

// ============================================================================
// Public API
// ============================================================================

void einkInit() {
    display.begin();
    display.setRotation(1);  // Landscape
    display.clearBuffer();
    display.setTextColor(EPD_BLACK);
    display.setTextWrap(false);

    // Splash screen
    display.setTextSize(2);
    display.setCursor(30, 30);
    display.print("AntiCant");
    display.setTextSize(1);
    display.setCursor(30, 60);
    display.print("Shooting Assistant v1.0");
    display.setCursor(30, 80);
    display.print("Initializing...");
    display.display();

    initialized = true;
    lastRefreshMs = millis();
    Serial.println("[EINK] Display initialized");
}

void einkClear() {
    if (!initialized) return;
    display.clearBuffer();
    display.display();
}

void einkUpdate(float cantAngle, DistancePreset distance, UserMode mode,
                uint16_t shotCount, uint8_t batteryPercent,
                bool forceRefresh) {

    if (!initialized) return;

    uint32_t now = millis();
    if (!forceRefresh && (now - lastRefreshMs < EINK_REFRESH_INTERVAL_MS)) {
        return;  // Not time to refresh yet
    }
    lastRefreshMs = now;

    display.clearBuffer();

    // ---- Layout ----
    // Row 1: CANT angle (large)         | Battery
    // Row 2: Distance | Tolerance
    // Row 3: Mode     | Shots

    // --- Row 1: Cant angle (large font) ---
    display.setTextSize(3);
    display.setCursor(4, 4);
    char cantBuf[16];
    snprintf(cantBuf, sizeof(cantBuf), "%+.1f", cantAngle);
    display.print("CANT ");
    display.print(cantBuf);
    display.setTextSize(1);
    display.print(" deg");

    // Battery (top-right corner)
    display.setTextSize(1);
    char batBuf[12];
    snprintf(batBuf, sizeof(batBuf), "BAT %d%%", batteryPercent);
    int16_t batX = EINK_WIDTH - (int16_t)(strlen(batBuf) * 6) - 4;
    display.setCursor(batX, 4);
    display.print(batBuf);

    // Battery icon (simple rectangle)
    int16_t iconX = batX - 18;
    display.drawRect(iconX, 2, 14, 8, EPD_BLACK);
    display.fillRect(iconX + 14, 4, 2, 4, EPD_BLACK);
    int fillW = (int)(12.0f * batteryPercent / 100.0f);
    if (fillW > 0) {
        display.fillRect(iconX + 1, 3, fillW, 6, EPD_BLACK);
    }

    drawDivider(34);

    // --- Row 2: Distance & Tolerance ---
    display.setTextSize(2);
    display.setCursor(4, 42);
    display.print(DISTANCE_VALUES[distance]);
    display.print(" m");

    display.setCursor(130, 42);
    char tolBuf[12];
    snprintf(tolBuf, sizeof(tolBuf), "+/-%.1f", TOLERANCE_VALUES[distance]);
    display.print(tolBuf);
    display.setTextSize(1);
    display.print(" deg");

    drawDivider(68);

    // --- Row 3: Mode & Shots ---
    display.setTextSize(2);
    display.setCursor(4, 78);
    display.print(MODE_NAMES[mode]);

    display.setCursor(150, 78);
    display.print(shotCount);
    display.setTextSize(1);
    display.setCursor(150, 98);
    display.print("shots");

    // --- Bottom bar: status indicators ---
    drawDivider(108);
    display.setTextSize(1);
    display.setCursor(4, 112);
    display.print("AntiCant v1.0");

    // Refresh indicator (dot)
    display.fillCircle(EINK_WIDTH - 6, 115, 2, EPD_BLACK);

    display.display();
}
