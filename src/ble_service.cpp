/*
 * ble_service.cpp — BLE Module implementation
 * Uses ArduinoBLE (nRF52840 native BLE stack).
 * Custom GATT service with 5 notify characteristics + 1 writable override.
 */

#include "ble_service.h"
#include <ArduinoBLE.h>

// ============================================================================
// BLE service & characteristics
// ============================================================================
static BLEService anticantService(BLE_SERVICE_UUID);

// Outgoing: device → phone (read + notify)
static BLECharacteristic charCant    (BLE_CHAR_CANT_UUID,     BLERead | BLENotify,              4);
static BLECharacteristic charMode    (BLE_CHAR_MODE_UUID,     BLERead | BLENotify,              1);
static BLECharacteristic charDistance(BLE_CHAR_DISTANCE_UUID, BLERead | BLENotify,              1);
static BLECharacteristic charShots   (BLE_CHAR_SHOTS_UUID,    BLERead | BLENotify,              2);
static BLECharacteristic charBattery (BLE_CHAR_BATTERY_UUID,  BLERead | BLENotify,              1);

// Incoming: phone → device (override angle as little-endian float)
static BLECharacteristic charOverride(BLE_CHAR_OVERRIDE_UUID, BLEWrite | BLEWriteWithoutResponse, 4);

static uint32_t lastNotifyMs    = 0;
static bool     bleReady        = false;
static bool     deviceConnected = false;

// Override angle from phone (-999 = no override)
static float    bleOverrideAngle  = -999.0f;
static bool     bleOverrideActive = false;

// ============================================================================
// Event handlers
// ============================================================================
static void onConnect(BLEDevice central) {
    deviceConnected = true;
    Serial.println("[BLE] Client connected");
}

static void onDisconnect(BLEDevice central) {
    deviceConnected   = false;
    bleOverrideActive = false;
    bleOverrideAngle  = -999.0f;
    Serial.println("[BLE] Client disconnected — restarting advertising");
    BLE.advertise();
}

static void onOverrideWritten(BLEDevice central, BLECharacteristic characteristic) {
    if (characteristic.valueLength() >= 4) {
        float val;
        memcpy(&val, characteristic.value(), 4);
        if (val < -900.0f) {
            bleOverrideActive = false;
            bleOverrideAngle  = -999.0f;
            Serial.println("[BLE] Override OFF");
        } else {
            bleOverrideActive = true;
            bleOverrideAngle  = val;
            SERIAL_PRINTF("[BLE] Override angle = %.1f°\n", val);
        }
    }
}

// ============================================================================
// Public API
// ============================================================================

void bleInit() {
    if (!BLE.begin()) {
        Serial.println("[BLE] ERROR: Failed to initialize BLE");
        return;
    }

    BLE.setLocalName(BLE_DEVICE_NAME);
    BLE.setAdvertisedService(anticantService);

    // Add characteristics to service
    anticantService.addCharacteristic(charCant);
    anticantService.addCharacteristic(charMode);
    anticantService.addCharacteristic(charDistance);
    anticantService.addCharacteristic(charShots);
    anticantService.addCharacteristic(charBattery);
    anticantService.addCharacteristic(charOverride);

    BLE.addService(anticantService);

    // Set initial values
    float    initCant  = 0.0f;
    uint8_t  initMode  = (uint8_t)MODE_TRAINING;
    uint8_t  initDist  = (uint8_t)DIST_100M;
    uint16_t initShots = 0;
    uint8_t  initBat   = 100;
    charCant    .writeValue((const uint8_t*)&initCant,  4);
    charMode    .writeValue(&initMode,  1);
    charDistance.writeValue(&initDist,  1);
    charShots   .writeValue((const uint8_t*)&initShots, 2);
    charBattery .writeValue(&initBat,   1);

    // Register event handlers
    BLE.setEventHandler(BLEConnected,    onConnect);
    BLE.setEventHandler(BLEDisconnected, onDisconnect);
    charOverride.setEventHandler(BLEWritten, onOverrideWritten);

    BLE.advertise();

    bleReady = true;
    Serial.println("[BLE] ArduinoBLE initialized, advertising as '" BLE_DEVICE_NAME "'");
}

void bleUpdate(float cantAngle, UserMode mode, DistancePreset distance,
               uint16_t shotCount, uint8_t batteryPercent) {

    if (!bleReady) return;

    // Always poll for events (connections, writes)
    BLE.poll();

    if (!deviceConnected) return;

    uint32_t now = millis();
    if (now - lastNotifyMs < BLE_NOTIFY_INTERVAL_MS) return;
    lastNotifyMs = now;

    // Write & notify all characteristics
    charCant.writeValue((const uint8_t*)&cantAngle, 4);

    uint8_t modeVal = (uint8_t)mode;
    charMode.writeValue(&modeVal, 1);

    uint8_t distVal = (uint8_t)distance;
    charDistance.writeValue(&distVal, 1);

    charShots.writeValue((const uint8_t*)&shotCount, 2);

    charBattery.writeValue(&batteryPercent, 1);
}

bool bleIsConnected() {
    return bleReady && deviceConnected;
}

void bleStartAdvertising() {
    if (bleReady) BLE.advertise();
}

void bleStopAdvertising() {
    if (bleReady) BLE.stopAdvertise();
}

bool bleHasOverride() {
    return bleOverrideActive;
}

float bleGetOverrideAngle() {
    return bleOverrideAngle;
}
