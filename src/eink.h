/*
 * eink.h — E-Ink Display Module (SSD1680 2.13")
 * Displays cant angle, distance, tolerance, mode, shots, battery.
 */

#ifndef EINK_H
#define EINK_H

#include <Arduino.h>
#include "config.h"

/// Initialize the e-ink display (SPI).
void einkInit();

/// Update the display content (respects 5s refresh interval internally).
/// @param cantAngle      Current filtered cant angle
/// @param distance       Active distance preset
/// @param mode           Active user mode
/// @param shotCount      Number of shots detected this session
/// @param batteryPercent Battery level 0-100%
/// @param forceRefresh   If true, bypass the 5s interval
void einkUpdate(float cantAngle, DistancePreset distance, UserMode mode,
                uint16_t shotCount, uint8_t batteryPercent,
                bool forceRefresh = false);

/// Clear the display to white.
void einkClear();

#endif // EINK_H
