/*
 * leds.cpp — WS2812B LED Module implementation
 * HSV interpolation, breathing green pulse, low-battery blink.
 */

#include "leds.h"
#include <Adafruit_NeoPixel.h>

// ============================================================================
// NeoPixel instance
// ============================================================================
static Adafruit_NeoPixel strip(NUM_LEDS, PIN_LEDS, NEO_GRB + NEO_KHZ800);

static uint8_t currentBrightness = LED_BRIGHTNESS_NORMAL;

// Tracking for green breathing pulse
static bool     inZone           = false;
static uint32_t zoneEnteredMs    = 0;
static bool     breathingActive  = false;

// Low battery blink state
static uint32_t lastBattBlinkMs  = 0;
static bool     battBlinkOn      = false;

// ============================================================================
// HSV → RGB helper (H: 0-65535, S: 0-255, V: 0-255)
// ============================================================================
static uint32_t hsvToColor(uint16_t h, uint8_t s, uint8_t v) {
    return strip.ColorHSV(h, s, v);
}

// ============================================================================
// Interpolation helper
// ============================================================================
static uint16_t lerpU16(uint16_t a, uint16_t b, float t) {
    if (t <= 0.0f) return a;
    if (t >= 1.0f) return b;
    return (uint16_t)((float)a + t * ((float)b - (float)a));
}

static uint8_t lerpU8(uint8_t a, uint8_t b, float t) {
    if (t <= 0.0f) return a;
    if (t >= 1.0f) return b;
    return (uint8_t)((float)a + t * ((float)b - (float)a));
}

// ============================================================================
// Breathing sine wave (0.0 → 1.0)
// ============================================================================
static float breathValue() {
    float phase = (float)(millis() % BREATH_PERIOD_MS) / (float)BREATH_PERIOD_MS;
    // Sine wave mapped to 0.3 → 1.0 (never fully off)
    float s = sinf(phase * 2.0f * PI);
    return 0.3f + 0.7f * (s * 0.5f + 0.5f);
}

// ============================================================================
// Public API
// ============================================================================

void ledsInit() {
    strip.begin();
    strip.setBrightness(currentBrightness);
    strip.clear();
    strip.show();
    Serial.println("[LEDS] NeoPixel initialized");
}

void ledsSetBrightness(uint8_t brightness) {
    currentBrightness = brightness;
    strip.setBrightness(currentBrightness);
}

uint8_t ledsGetBrightness() {
    return currentBrightness;
}

void ledsOff() {
    strip.clear();
    strip.show();
}

void ledsUpdate(float cantAngle, float tolerance, UserMode mode, bool batteryLow) {
    // --- Low battery override: slow red blink ---
    if (batteryLow) {
        uint32_t now = millis();
        if (now - lastBattBlinkMs > 1000) {
            lastBattBlinkMs = now;
            battBlinkOn = !battBlinkOn;
        }
        uint32_t color = battBlinkOn ? strip.Color(255, 0, 0) : 0;
        for (int i = 0; i < NUM_LEDS; i++) {
            strip.setPixelColor(i, color);
        }
        strip.show();
        return;
    }

    // --- Brightness per mode ---
    switch (mode) {
        case MODE_COMPETITION:
            ledsSetBrightness(LED_BRIGHTNESS_MAX);
            break;
        case MODE_DISCRET:
            ledsSetBrightness(LED_BRIGHTNESS_DISCRET);
            break;
        default:
            ledsSetBrightness(LED_BRIGHTNESS_NORMAL);
            break;
    }

    float absCant = fabsf(cantAngle);
    float approachZone = tolerance * APPROACH_ZONE_MULT;

    // HSV hue values (NeoPixel: 0=Red, ~10922=Green, ~5461=Yellow/Orange)
    // Red:    H=0
    // Orange: H=5461  (~30°)
    // Green:  H=10922 (~60° in 16-bit space, actually 21845 = 120° → green)
    const uint16_t HUE_RED    = 0;
    const uint16_t HUE_ORANGE = 5461;    // ~30°
    const uint16_t HUE_GREEN  = 21845;   // 120° = pure green

    uint16_t hue;
    uint8_t  sat = 255;
    uint8_t  val = 255;

    if (absCant <= tolerance) {
        // ===== IN THE VALID ZONE =====
        hue = HUE_GREEN;

        // Track how long we've been in zone
        if (!inZone) {
            inZone = true;
            zoneEnteredMs = millis();
            breathingActive = false;
        }

        // Activate breathing after 1s in zone
        if (!breathingActive && (millis() - zoneEnteredMs >= GREEN_HOLD_THRESHOLD_MS)) {
            breathingActive = true;
        }

        if (breathingActive) {
            val = (uint8_t)(breathValue() * 255.0f);
        }

    } else if (absCant <= approachZone) {
        // ===== APPROACH ZONE: interpolate orange → green =====
        inZone = false;
        breathingActive = false;

        // t=0 at edge of approach zone, t=1 at tolerance boundary
        float t = 1.0f - (absCant - tolerance) / (approachZone - tolerance);
        t = constrain(t, 0.0f, 1.0f);

        hue = lerpU16(HUE_ORANGE, HUE_GREEN, t);

    } else {
        // ===== OUT OF ZONE: red (with slight orange blend near boundary) =====
        inZone = false;
        breathingActive = false;

        // Interpolate red → orange as we get closer to approach zone
        float maxRange = approachZone * 2.0f;
        if (maxRange < 5.0f) maxRange = 5.0f;

        float t = 1.0f - (absCant - approachZone) / (maxRange - approachZone);
        t = constrain(t, 0.0f, 1.0f);

        hue = lerpU16(HUE_RED, HUE_ORANGE, t);
    }

    // Apply color to all LEDs
    uint32_t color = hsvToColor(hue, sat, val);
    for (int i = 0; i < NUM_LEDS; i++) {
        strip.setPixelColor(i, color);
    }
    strip.show();
}
