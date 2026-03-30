/*
 * ble_service.cpp — BLE Module implementation
 * Uses NimBLE-Arduino (ESP32 native BLE stack).
 * Custom GATT service with 5 characteristics + notify.
 */

#include "ble_service.h"
#include <NimBLEDevice.h>

// ============================================================================
// NimBLE objects
// ============================================================================
static NimBLEServer*          pServer       = nullptr;
static NimBLEService*         pService      = nullptr;
static NimBLECharacteristic*  pCharCant     = nullptr;
static NimBLECharacteristic*  pCharMode     = nullptr;
static NimBLECharacteristic*  pCharDistance  = nullptr;
static NimBLECharacteristic*  pCharShots    = nullptr;
static NimBLECharacteristic*  pCharBattery  = nullptr;

static uint32_t lastNotifyMs    = 0;
static bool     bleReady        = false;
static bool     deviceConnected = false;

// ============================================================================
// Connection callbacks
// ============================================================================
class ServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pSrv) override {
        deviceConnected = true;
        Serial.println("[BLE] Client connected");
    }
    void onDisconnect(NimBLEServer* pSrv) override {
        deviceConnected = false;
        Serial.println("[BLE] Client disconnected — restarting advertising");
        NimBLEDevice::startAdvertising();
    }
};

// ============================================================================
// Public API
// ============================================================================

void bleInit() {
    NimBLEDevice::init(BLE_DEVICE_NAME);
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);  // Max TX power

    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());

    pService = pServer->createService(BLE_SERVICE_UUID);

    // Create characteristics (read + notify)
    pCharCant = pService->createCharacteristic(
        BLE_CHAR_CANT_UUID,
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
    );
    pCharMode = pService->createCharacteristic(
        BLE_CHAR_MODE_UUID,
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
    );
    pCharDistance = pService->createCharacteristic(
        BLE_CHAR_DISTANCE_UUID,
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
    );
    pCharShots = pService->createCharacteristic(
        BLE_CHAR_SHOTS_UUID,
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
    );
    pCharBattery = pService->createCharacteristic(
        BLE_CHAR_BATTERY_UUID,
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
    );

    // Set initial values
    float initCant = 0.0f;
    pCharCant->setValue(initCant);
    uint8_t initMode = (uint8_t)MODE_TRAINING;
    pCharMode->setValue(&initMode, 1);
    uint8_t initDist = (uint8_t)DIST_100M;
    pCharDistance->setValue(&initDist, 1);
    uint16_t initShots = 0;
    pCharShots->setValue(initShots);
    uint8_t initBat = 100;
    pCharBattery->setValue(&initBat, 1);

    // Start service & advertising
    pService->start();

    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(BLE_SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->start();

    bleReady = true;
    Serial.println("[BLE] NimBLE initialized, advertising as '" BLE_DEVICE_NAME "'");
}

void bleUpdate(float cantAngle, UserMode mode, DistancePreset distance,
               uint16_t shotCount, uint8_t batteryPercent) {

    if (!bleReady || !deviceConnected) return;

    // Respect notify interval
    uint32_t now = millis();
    if (now - lastNotifyMs < BLE_NOTIFY_INTERVAL_MS) {
        return;
    }
    lastNotifyMs = now;

    // Write values & notify subscribed clients
    pCharCant->setValue(cantAngle);
    pCharCant->notify();

    uint8_t modeVal = (uint8_t)mode;
    pCharMode->setValue(&modeVal, 1);
    pCharMode->notify();

    uint8_t distVal = (uint8_t)distance;
    pCharDistance->setValue(&distVal, 1);
    pCharDistance->notify();

    pCharShots->setValue(shotCount);
    pCharShots->notify();

    pCharBattery->setValue(&batteryPercent, 1);
    pCharBattery->notify();
}

bool bleIsConnected() {
    return bleReady && deviceConnected;
}

void bleStartAdvertising() {
    if (bleReady) NimBLEDevice::startAdvertising();
}

void bleStopAdvertising() {
    if (bleReady) NimBLEDevice::stopAdvertising();
}
