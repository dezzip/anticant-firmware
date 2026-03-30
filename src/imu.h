/*
 * imu.h — IMU Module (LSM6DS3) with Kalman filter
 * Reads roll (cant) angle at 60 Hz.
 */

#ifndef IMU_H
#define IMU_H

#include <Arduino.h>

// ============================================================================
// Kalman Filter State
// ============================================================================
struct KalmanState {
    float angle;        // Estimated angle
    float bias;         // Estimated gyro bias
    float rate;         // Unbiased rate
    float P[2][2];      // Error covariance matrix
    float Q_angle;
    float Q_bias;
    float R_measure;
};

// ============================================================================
// Public API
// ============================================================================

/// Initialize the LSM6DS3 IMU and Kalman filter.
/// Returns true on success.
bool imuInit();

/// Update the IMU reading and run Kalman filter.
/// Call this at IMU_UPDATE_HZ (60 Hz).
void imuUpdate();

/// Get the current filtered cant angle in degrees.
/// Positive = clockwise tilt, negative = counter-clockwise.
float imuGetCantAngle();

/// Get raw acceleration magnitude (for shot detection).
float imuGetAccelMagnitude();

/// Get raw accelerometer values (x, y, z in g).
void imuGetRawAccel(float &ax, float &ay, float &az);

/// Get raw gyroscope values (x, y, z in deg/s).
void imuGetRawGyro(float &gx, float &gy, float &gz);

#endif // IMU_H
