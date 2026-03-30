/*
 * battery.cpp — Battery Management implementation
 * Reads LiPo voltage via analog pin with voltage divider.
 * Smoothed with a simple moving average.
 */

#include "battery.h"
#include "config.h"

// ============================================================================
// Private state
// ============================================================================
static float    voltage      = 4.2f;
static uint8_t  percent      = 100;
static bool     lowBattery   = false;
static uint32_t lastReadMs   = 0;

// Simple moving average buffer
#define BATT_AVG_SAMPLES  8
static float    samples[BATT_AVG_SAMPLES];
static uint8_t  sampleIdx = 0;
static bool     bufferFull = false;

// ============================================================================
// LiPo discharge curve approximation (voltage → percent)
// ============================================================================
static uint8_t voltageToPercent(float v) {
    // Piecewise linear approximation of LiPo discharge curve
    if (v >= 4.20f) return 100;
    if (v >= 4.06f) return 90;
    if (v >= 3.98f) return 80;
    if (v >= 3.92f) return 70;
    if (v >= 3.87f) return 60;
    if (v >= 3.82f) return 50;
    if (v >= 3.79f) return 40;
    if (v >= 3.77f) return 30;
    if (v >= 3.74f) return 20;
    if (v >= 3.68f) return 10;
    if (v >= 3.45f) return 5;
    if (v >= BATTERY_V_MIN) return 1;
    return 0;
}

// ============================================================================
// Public API
// ============================================================================

void batteryInit() {
    analogReadResolution(12);  // 12-bit ADC (0-4095)
    pinMode(PIN_BATTERY, INPUT);

    // Pre-fill average buffer
    for (int i = 0; i < BATT_AVG_SAMPLES; i++) {
        float v = (float)analogReadMilliVolts(PIN_BATTERY) / 1000.0f * BATTERY_DIVIDER_RATIO;
        samples[i] = v;
    }
    bufferFull = true;

    // Compute initial values
    float sum = 0;
    for (int i = 0; i < BATT_AVG_SAMPLES; i++) sum += samples[i];
    voltage    = sum / BATT_AVG_SAMPLES;
    percent    = voltageToPercent(voltage);
    lowBattery = (percent < BATTERY_LOW_THRESHOLD);

    lastReadMs = millis();
    Serial.printf("[BATT] Init: %.2fV → %d%%\n", voltage, percent);
}

void batteryUpdate() {
    uint32_t now = millis();
    if (now - lastReadMs < BATTERY_READ_INTERVAL_MS) return;
    lastReadMs = now;

    // Read ADC (ESP32S3 — lecture calibrée en millivolts)
    float v = (float)analogReadMilliVolts(PIN_BATTERY) / 1000.0f * BATTERY_DIVIDER_RATIO;

    // Add to moving average
    samples[sampleIdx] = v;
    sampleIdx = (sampleIdx + 1) % BATT_AVG_SAMPLES;
    if (sampleIdx == 0) bufferFull = true;

    // Compute average
    int count = bufferFull ? BATT_AVG_SAMPLES : sampleIdx;
    float sum = 0;
    for (int i = 0; i < count; i++) sum += samples[i];
    voltage = sum / (float)count;

    // Update percent & low flag
    percent    = voltageToPercent(voltage);
    lowBattery = (percent < BATTERY_LOW_THRESHOLD);
}

uint8_t batteryGetPercent() {
    return percent;
}

float batteryGetVoltage() {
    return voltage;
}

bool batteryIsLow() {
    return lowBattery;
}
