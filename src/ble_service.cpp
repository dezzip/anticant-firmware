/*
 * ble_service.cpp — BLE Module implementation
 * Uses ArduinoBLE (built-in on nRF52840).
 * Custom GATT service with 5 characteristics + notify.
 */

#include "ble_service.h"
#include <ArduinoBLE.h>

// ============================================================================
// BLE Service & Characteristics
// ============================================================================
static BLEService anticantService(BLE_SERVICE_UUID);

// Characteristics (read + notify)
static BLEFloatCharacteristic   charCant(BLE_CHAR_CANT_UUID, BLERead | BLENotify);
static BLEByteCharacteristic    charMode(BLE_CHAR_MODE_UUID, BLERead | BLENotify);
static BLEByteCharacteristic    charDistance(BLE_CHAR_DISTANCE_UUID, BLERead | BLENotify);
static BLEUnsignedShortCharacteristic charShots(BLE_CHAR_SHOTS_UUID, BLERead | BLENotify);
static BLEByteCharacteristic    charBattery(BLE_CHAR_BATTERY_UUID, BLERead | BLENotify);

static uint32_t lastNotifyMs = 0;
static bool     bleReady     = false;

// ============================================================================
// Public API
// ============================================================================

void bleInit() {
    if (!BLE.begin()) {
        Serial.println("[BLE] ERROR: BLE init failed!");
        bleReady = false;
        return;
    }

    // Set device name and local name
    BLE.setLocalName(BLE_DEVICE_NAME);
    BLE.setDeviceName(BLE_DEVICE_NAME);
    BLE.setAdvertisedService(anticantService);

    // Add characteristics to service
    anticantService.addCharacteristic(charCant);
    anticantService.addCharacteristic(charMode);
    anticantService.addCharacteristic(charDistance);
    anticantService.addCharacteristic(charShots);
    anticantService.addCharacteristic(charBattery);

    // Add service
    BLE.addService(anticantService);

    // Set initial values
    charCant.writeValue(0.0f);
    charMode.writeValue((uint8_t)MODE_TRAINING);
    charDistance.writeValue((uint8_t)DIST_100M);
    charShots.writeValue((uint16_t)0);
    charBattery.writeValue((uint8_t)100);

    // Start advertising
    BLE.advertise();

    bleReady = true;
    Serial.println("[BLE] Initialized, advertising as '" BLE_DEVICE_NAME "'");
}

void bleUpdate(float cantAngle, UserMode mode, DistancePreset distance,
               uint16_t shotCount, uint8_t batteryPercent) {

    if (!bleReady) return;

    // Poll BLE events
    BLE.poll();

    // Check if central is connected
    BLEDevice central = BLE.central();
    if (!central || !central.connected()) {
        return;
    }

    // Respect notify interval
    uint32_t now = millis();
    if (now - lastNotifyMs < BLE_NOTIFY_INTERVAL_MS) {
        return;
    }
    lastNotifyMs = now;

    // Write values → triggers notify to subscribed client
    charCant.writeValue(cantAngle);
    charMode.writeValue((uint8_t)mode);
    charDistance.writeValue((uint8_t)distance);
    charShots.writeValue(shotCount);
    charBattery.writeValue(batteryPercent);
}

bool bleIsConnected() {
    if (!bleReady) return false;
    BLEDevice central = BLE.central();
    return (central && central.connected());
}

void bleStartAdvertising() {
    if (bleReady) BLE.advertise();
}

void bleStopAdvertising() {
    if (bleReady) BLE.stopAdvertise();
}
