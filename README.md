# AntiCant — Shooting Assistant Firmware

Firmware embarqué pour assistant de tir anti-cant basé sur le **Seeed Studio XIAO ESP32S3**.

## Hardware

| Composant | Détail |
|-----------|--------|
| MCU | Seeed Studio XIAO ESP32S3 |
| IMU | LSM6DS3 (accéléromètre + gyroscope 6 axes, I2C externe) |
| LEDs | 6× WS2812B adressables (pin D0) |
| Écran | E-ink 2.13" SSD1680 (SPI) |
| Bouton | Pin D1 (INPUT_PULLUP) |
| Batterie | LiPo avec charge USB-C intégrée |

## Fonctionnalités

- **Lecture IMU** — Angle de cant en temps réel (roll) avec filtre de Kalman à 60 Hz
- **Feedback LED** — 6 LEDs WS2812B avec interpolation HSV fluide (rouge → orange → vert)
- **Sensibilité adaptative** — Tolérance ajustable par distance (100m / 300m / 600m / 1000m)
- **Modes utilisateur** — TRAINING / COMPETITION / DISCRET
- **Écran e-ink** — Distance, tolérance, mode, tirs, batterie, angle cant
- **Détection de tir** — Compteur automatique par pic d'accélération IMU
- **BLE** — Service Bluetooth Low Energy avec notifications toutes les 500 ms
- **Gestion batterie** — Lecture tension + alerte LED si < 15%

## Structure du code

```
src/
├── main.ino              # Fichier principal
├── config.h              # Configuration globale
├── imu.h / imu.cpp       # Module IMU + filtre Kalman
├── leds.h / leds.cpp     # Module LEDs WS2812B
├── eink.h / eink.cpp     # Module écran e-ink
├── ble_service.h / .cpp  # Module BLE
├── battery.h / .cpp      # Module batterie
└── shot_detector.h/.cpp  # Module détection de tir
```

## Build & Flash

```bash
# Build
pio run

# Flash
pio run --target upload

# Monitor série
pio device monitor
```

## Contrôles

| Action | Commande |
|--------|----------|
| Appui court | Cycle distance (100m → 300m → 600m → 1000m) |
| Appui long (>2s) | Cycle mode (TRAINING → COMPETITION → DISCRET) |
| Double appui | Reset compteur de tirs |

## Licence

MIT
