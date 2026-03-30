/*
 * shot_detector.h — Shot Detection Module
 * Detects shots via IMU acceleration spikes.
 */

#ifndef SHOT_DETECTOR_H
#define SHOT_DETECTOR_H

#include <Arduino.h>

/// Initialize the shot detector.
void shotInit();

/// Update shot detection. Call every IMU cycle.
/// @param accelMagnitude  Current acceleration magnitude (g)
void shotUpdate(float accelMagnitude);

/// Get the number of shots detected this session.
uint16_t shotGetCount();

/// Reset the shot counter to zero.
void shotReset();

/// Returns true if a shot was detected on this update cycle.
bool shotDetectedThisCycle();

#endif // SHOT_DETECTOR_H
