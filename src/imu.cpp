/*
 * imu.cpp — IMU Module implementation
 * LSM6DS3 on Seeed XIAO BLE Sense (I2C, built-in)
 * Kalman-filtered roll angle for cant measurement.
 */

#include "imu.h"
#include "config.h"
#include <LSM6DS3.h>
#include <Wire.h>

// ============================================================================
// Private state
// ============================================================================
static LSM6DS3 imuSensor(I2C_MODE, 0x6A);  // Default I2C address on XIAO Sense

static KalmanState kalman;
static float cantAngle   = 0.0f;
static float accelMag    = 1.0f;
static float rawAx, rawAy, rawAz;
static float rawGx, rawGy, rawGz;
static uint32_t lastUpdateUs = 0;

// ============================================================================
// Kalman helpers
// ============================================================================

static void kalmanInit(KalmanState &k, float initAngle) {
    k.angle     = initAngle;
    k.bias      = 0.0f;
    k.rate      = 0.0f;
    k.Q_angle   = KALMAN_Q_ANGLE;
    k.Q_bias    = KALMAN_Q_BIAS;
    k.R_measure = KALMAN_R_MEASURE;
    k.P[0][0]   = 0.0f;
    k.P[0][1]   = 0.0f;
    k.P[1][0]   = 0.0f;
    k.P[1][1]   = 0.0f;
}

static float kalmanUpdate(KalmanState &k, float newAngle, float newRate, float dt) {
    // ---- Predict ----
    k.rate   = newRate - k.bias;
    k.angle += dt * k.rate;

    // Update error covariance
    k.P[0][0] += dt * (dt * k.P[1][1] - k.P[0][1] - k.P[1][0] + k.Q_angle);
    k.P[0][1] -= dt * k.P[1][1];
    k.P[1][0] -= dt * k.P[1][1];
    k.P[1][1] += k.Q_bias * dt;

    // ---- Update (measurement) ----
    float S = k.P[0][0] + k.R_measure;     // Innovation covariance
    float K[2];                              // Kalman gain
    K[0] = k.P[0][0] / S;
    K[1] = k.P[1][0] / S;

    float y = newAngle - k.angle;            // Innovation
    k.angle += K[0] * y;
    k.bias  += K[1] * y;

    float P00_tmp = k.P[0][0];
    float P01_tmp = k.P[0][1];
    k.P[0][0] -= K[0] * P00_tmp;
    k.P[0][1] -= K[0] * P01_tmp;
    k.P[1][0] -= K[1] * P00_tmp;
    k.P[1][1] -= K[1] * P01_tmp;

    return k.angle;
}

// ============================================================================
// Public API
// ============================================================================

bool imuInit() {
    if (imuSensor.begin() != 0) {
        Serial.println("[IMU] ERROR: LSM6DS3 init failed!");
        return false;
    }
    Serial.println("[IMU] LSM6DS3 initialized OK");

    // Read initial orientation to seed Kalman
    float ax = imuSensor.readFloatAccelX();
    float ay = imuSensor.readFloatAccelY();
    float az = imuSensor.readFloatAccelZ();
    float initRoll = atan2f(ay, az) * 180.0f / PI;

    kalmanInit(kalman, initRoll);
    cantAngle    = initRoll;
    lastUpdateUs = micros();

    return true;
}

void imuUpdate() {
    uint32_t now = micros();
    float dt = (float)(now - lastUpdateUs) / 1000000.0f;
    if (dt <= 0.0f) dt = 1.0f / IMU_UPDATE_HZ;
    lastUpdateUs = now;

    // Read raw accelerometer (g)
    rawAx = imuSensor.readFloatAccelX();
    rawAy = imuSensor.readFloatAccelY();
    rawAz = imuSensor.readFloatAccelZ();

    // Read raw gyroscope (deg/s)
    rawGx = imuSensor.readFloatGyroX();
    rawGy = imuSensor.readFloatGyroY();
    rawGz = imuSensor.readFloatGyroZ();

    // Acceleration magnitude (for shot detection)
    accelMag = sqrtf(rawAx * rawAx + rawAy * rawAy + rawAz * rawAz);

    // Roll angle from accelerometer
    float accelRoll = atan2f(rawAy, rawAz) * 180.0f / PI;

    // Gyro rate around X axis (roll rate)
    float gyroRateX = rawGx;

    // Kalman filter fusion
    cantAngle = kalmanUpdate(kalman, accelRoll, gyroRateX, dt);
}

float imuGetCantAngle() {
    return cantAngle;
}

float imuGetAccelMagnitude() {
    return accelMag;
}

void imuGetRawAccel(float &ax, float &ay, float &az) {
    ax = rawAx;
    ay = rawAy;
    az = rawAz;
}

void imuGetRawGyro(float &gx, float &gy, float &gz) {
    gx = rawGx;
    gy = rawGy;
    gz = rawGz;
}
