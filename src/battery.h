/*
 * battery.h — Battery Management Module
 * Reads LiPo voltage via ADC and maps to percentage.
 */

#ifndef BATTERY_H
#define BATTERY_H

#include <Arduino.h>

/// Initialize the battery ADC pin.
void batteryInit();

/// Update battery reading (respects internal interval).
void batteryUpdate();

/// Get battery percentage (0-100%).
uint8_t batteryGetPercent();

/// Get battery voltage (V).
float batteryGetVoltage();

/// Returns true if battery is below low threshold (15%).
bool batteryIsLow();

#endif // BATTERY_H
