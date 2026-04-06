/*
 * eink.cpp — E-Ink Display Module implementation
 * 1.54" e-paper (SSD1680, 200×200 pixels) via SPI.
 * Splash with dinosaur logo (7s), then clean 4-row main UI.
 */

#include "eink.h"
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_EPD.h>

// ============================================================================
// Display instance — 1.54" V2 = SSD1681, 200×200 pixels
// ============================================================================
static Adafruit_SSD1681 display(
    EINK_WIDTH, EINK_HEIGHT,
    PIN_EINK_DC, PIN_EINK_RST, PIN_EINK_CS,
    -1,   // SRCS — no external SRAM
    -1    // BUSY — not connected, internal delays
);

static uint32_t lastRefreshMs = 0;
static bool     initialized   = false;

#define W  EINK_WIDTH    // 200
#define H  EINK_HEIGHT   // 200

// ============================================================================
// Helpers
// ============================================================================

static void drawThickLine(int16_t y, int16_t thick = 2) {
    display.fillRect(4, y, W - 8, thick, EPD_BLACK);
}

static int16_t rightX(int len, uint8_t textSize, int16_t margin = 10) {
    return W - margin - (int16_t)(len * 6 * textSize);
}

// Center text horizontally
static int16_t centerX(int len, uint8_t textSize) {
    return (W - (int16_t)(len * 6 * textSize)) / 2;
}

// ============================================================================
// Dinosaur logo — drawn with GFX shapes (~58×55px)
// ============================================================================
static void drawDino(int16_t x, int16_t y) {
    // Head
    display.fillRoundRect(x+30, y+0,  26, 18, 3, EPD_BLACK);
    display.fillCircle(x+46, y+7, 4, EPD_WHITE);
    display.fillCircle(x+47, y+7, 2, EPD_BLACK);
    display.fillRect(x+40, y+14, 16, 4, EPD_WHITE);
    display.fillRect(x+42, y+13, 2, 3, EPD_WHITE);
    display.fillRect(x+48, y+13, 2, 3, EPD_WHITE);

    // Neck
    display.fillRect(x+30, y+16, 12, 8, EPD_BLACK);

    // Body
    display.fillRoundRect(x+12, y+20, 32, 24, 5, EPD_BLACK);

    // Tail
    display.fillTriangle(x+12, y+24,  x+12, y+38,  x+0, y+16, EPD_BLACK);
    display.fillTriangle(x+0, y+16,  x+0, y+22,  x+6, y+20, EPD_BLACK);

    // Tiny arm
    display.fillRect(x+42, y+27, 10, 3, EPD_BLACK);
    display.fillRect(x+50, y+30, 3, 3, EPD_BLACK);

    // Left leg
    display.fillRect(x+18, y+42, 8, 11, EPD_BLACK);
    display.fillRect(x+14, y+51, 14, 4, EPD_BLACK);

    // Right leg
    display.fillRect(x+32, y+42, 8, 11, EPD_BLACK);
    display.fillRect(x+28, y+51, 14, 4, EPD_BLACK);
}

// ============================================================================
// Splash screen — 7 seconds
// ============================================================================
static void showSplash() {
    // Single white clear — less flashy than double black clear
    display.fillScreen(EPD_WHITE);
    display.display();
    delay(300);

    display.clearBuffer();
    display.setTextColor(EPD_BLACK);
    display.setTextWrap(false);

    // Frame border
    display.drawRect(0, 0, W, H, EPD_BLACK);
    display.drawRect(1, 1, W-2, H-2, EPD_BLACK);

    // Dinosaur centered
    drawDino(70, 25);

    // "ANTICANT" large centered
    display.setTextSize(3);  // 8 chars × 18px = 144px → center: (200-144)/2 = 28
    display.setCursor(28, 92);
    display.print("ANTICANT");

    // Tagline
    display.setTextSize(1);
    display.setCursor(centerX(18, 1), 122);  // "Shooting Assistant"
    display.print("Shooting Assistant");

    // Thin decorative lines
    display.drawFastHLine(20, 118, W - 40, EPD_BLACK);
    display.drawFastHLine(20, 140, W - 40, EPD_BLACK);

    // Version bottom-left
    display.setTextSize(1);
    display.setCursor(8, H - 14);
    display.print("v1.0");

    display.display();
    Serial.println("[EINK] Splash displayed");

    delay(7000);
    Serial.println("[EINK] Splash done");
}

// ============================================================================
// Main UI — 200×200 square layout
// ============================================================================
//
//  ┌────────────────────────────────┐
//  │       +2.3°          LEVEL    │  Row 1: Cant angle + status
//  │════════════════════════════════│
//  │  300 m              90 MOA    │  Row 2: Distance + MOA
//  │════════════════════════════════│
//  │  TRAINING           12 tirs   │  Row 3: Mode + shots
//  │════════════════════════════════│
//  │  🔋 0%                  v1.0  │  Row 4: Battery + version
//  └────────────────────────────────┘
//

static void drawMainUI(float cantAngle, DistancePreset distance,
                        UserMode mode, uint16_t shotCount,
                        uint8_t batteryPercent) {
    display.fillScreen(EPD_WHITE);  // White clear — less flash than black
    display.setTextColor(EPD_BLACK);
    display.setTextWrap(false);

    // ===== OUTER FRAME =====
    display.drawRect(0, 0, W, H, EPD_BLACK);
    display.drawRect(1, 1, W-2, H-2, EPD_BLACK);

    // Layout: 4 rows in 200px height
    // Row 1: y=6..48   (cant angle — big)  height ~42
    // Line:  y=49
    // Row 2: y=55..95  (distance + MOA)    height ~40
    // Line:  y=97
    // Row 3: y=103..143 (mode + shots)     height ~40
    // Line:  y=145
    // Row 4: y=151..190 (battery + ver)    height ~40

    // ===== ROW 1: CANT ANGLE (big) + status =====
    display.setTextSize(3);  // 18×24 per char
    char angleBuf[12];
    snprintf(angleBuf, sizeof(angleBuf), "%+.1f", cantAngle);
    display.setCursor(10, 12);
    display.print(angleBuf);
    // Degree symbol
    int16_t cx = display.getCursorX() + 3;
    display.fillCircle(cx, 14, 3, EPD_BLACK);
    display.fillCircle(cx, 14, 1, EPD_WHITE);

    // Status label
    float absCant = fabsf(cantAngle);
    float tol = TOLERANCE_VALUES[distance];
    display.setTextSize(2);
    if (absCant <= tol) {
        display.setCursor(rightX(5, 2), 16);
        display.print("LEVEL");
    } else {
        display.setCursor(rightX(5, 2), 16);
        display.print("CANT!");
    }

    drawThickLine(49);

    // ===== ROW 2: DISTANCE + MOA =====
    display.setTextSize(2);
    display.setCursor(10, 60);
    display.print(DISTANCE_VALUES[distance]);
    display.print(" m");

    float moa = tol * 60.0f;
    char moaBuf[14];
    snprintf(moaBuf, sizeof(moaBuf), "%.0f MOA", moa);
    display.setCursor(rightX(strlen(moaBuf), 2), 60);
    display.print(moaBuf);

    // Tolerance in degrees (smaller, below)
    display.setTextSize(1);
    char tolBuf[16];
    snprintf(tolBuf, sizeof(tolBuf), "+/- %.1f deg", tol);
    display.setCursor(rightX(strlen(tolBuf), 1), 80);
    display.print(tolBuf);

    drawThickLine(97);

    // ===== ROW 3: MODE + SHOTS =====
    display.setTextSize(2);
    display.setCursor(10, 108);
    display.print(MODE_NAMES[mode]);

    char shotBuf[14];
    snprintf(shotBuf, sizeof(shotBuf), "%d tirs", shotCount);
    display.setCursor(rightX(strlen(shotBuf), 2), 108);
    display.print(shotBuf);

    drawThickLine(145);

    // ===== ROW 4: BATTERY + VERSION =====
    // Battery icon (26×14)
    display.drawRect(10, 157, 24, 14, EPD_BLACK);
    display.fillRect(34, 161, 3, 6, EPD_BLACK);
    int fillW = (int)(22.0f * batteryPercent / 100.0f);
    if (fillW > 0) {
        display.fillRect(11, 158, fillW, 12, EPD_BLACK);
    }

    // Battery percentage
    display.setTextSize(2);
    char batBuf[10];
    snprintf(batBuf, sizeof(batBuf), "%d%%", batteryPercent);
    display.setCursor(42, 157);
    display.print(batBuf);

    // Version (right side, small)
    display.setTextSize(1);
    display.setCursor(rightX(4, 1), 165);
    display.print("v1.0");

    display.display();
}

// ============================================================================
// Public API
// ============================================================================

void einkInit() {
    display.begin();
    display.setRotation(1);  // Rotated +180° from previous

    Serial.println("[EINK] Init...");
    showSplash();

    initialized = true;
    lastRefreshMs = millis();
    Serial.println("[EINK] Ready");
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
        return;
    }
    lastRefreshMs = now;

    drawMainUI(cantAngle, distance, mode, shotCount, batteryPercent);
}
