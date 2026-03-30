/*
 * leds.h — WS2812B LED Module
 * 6 LEDs with HSV interpolation, breathing pulse, brightness levels.
 */

#ifndef LEDS_H
#define LEDS_H

#include <Arduino.h>
#include "config.h"

/// Initialize NeoPixel strip.
void ledsInit();

/// Update LED feedback based on cant angle and current settings.
/// @param cantAngle   Current filtered cant angle (degrees)
/// @param tolerance   Active tolerance (degrees, e.g. 0.4)
/// @param mode        Current user mode
/// @param batteryLow  True if battery < 15%
void ledsUpdate(float cantAngle, float tolerance, UserMode mode, bool batteryLow);

/// Force all LEDs off.
void ledsOff();

/// Set brightness level directly (0-255).
void ledsSetBrightness(uint8_t brightness);

/// Get current brightness.
uint8_t ledsGetBrightness();

#endif // LEDS_H
