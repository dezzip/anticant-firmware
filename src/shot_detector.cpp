/*
 * shot_detector.cpp — Shot Detection implementation
 * Detects recoil spikes in IMU acceleration data.
 * Uses a threshold + cooldown to avoid double-counting.
 */

#include "shot_detector.h"
#include "config.h"

// ============================================================================
// Private state
// ============================================================================
static uint16_t shotCount     = 0;
static bool     detected      = false;
static uint32_t lastShotMs    = 0;
static bool     aboveThresh   = false;

// ============================================================================
// Public API
// ============================================================================

void shotInit() {
    shotCount   = 0;
    detected    = false;
    lastShotMs  = 0;
    aboveThresh = false;
    Serial.println("[SHOT] Detector initialized");
}

void shotUpdate(float accelMagnitude) {
    detected = false;  // Reset per-cycle flag
    uint32_t now = millis();

    // Simple peak detection with cooldown
    if (accelMagnitude >= SHOT_ACCEL_THRESHOLD) {
        if (!aboveThresh && (now - lastShotMs >= SHOT_COOLDOWN_MS)) {
            // Rising edge above threshold → shot detected
            shotCount++;
            detected   = true;
            lastShotMs = now;
            Serial.printf("[SHOT] Detected! Count: %d (accel: %.2f g)\n",
                          shotCount, accelMagnitude);
        }
        aboveThresh = true;
    } else {
        aboveThresh = false;
    }
}

uint16_t shotGetCount() {
    return shotCount;
}

void shotReset() {
    shotCount = 0;
    Serial.println("[SHOT] Counter reset");
}

bool shotDetectedThisCycle() {
    return detected;
}
