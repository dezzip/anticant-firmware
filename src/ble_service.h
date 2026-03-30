/*
 * ble_service.h — BLE Module (Bluetooth Low Energy)
 * Custom GATT service for AntiCant data streaming.
 */

#ifndef BLE_SERVICE_H
#define BLE_SERVICE_H

#include <Arduino.h>
#include "config.h"

/// Initialize BLE peripheral with custom service and characteristics.
void bleInit();

/// Update BLE and send notifications if a client is connected.
/// Call this in the main loop; it respects the 500ms notify interval internally.
/// @param cantAngle      Current cant angle
/// @param mode           Current user mode
/// @param distance       Active distance preset
/// @param shotCount      Shot counter
/// @param batteryPercent Battery percentage
void bleUpdate(float cantAngle, UserMode mode, DistancePreset distance,
               uint16_t shotCount, uint8_t batteryPercent);

/// Returns true if a BLE central (client) is currently connected.
bool bleIsConnected();

/// Start/stop BLE advertising.
void bleStartAdvertising();
void bleStopAdvertising();

#endif // BLE_SERVICE_H
