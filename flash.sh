#!/bin/bash
# flash.sh — Build + Flash + Monitor pour AntiCant sur XIAO BLE Sense
# Usage: ./flash.sh [--no-build] [--no-monitor]

set -e

FIRMWARE_DIR="$(cd "$(dirname "$0")" && pwd)"
ZEPHYR_DIR="$HOME/zephyrproject"
BUILD_DIR="$ZEPHYR_DIR/build"
UF2_FILE="$BUILD_DIR/zephyr/zephyr.uf2"
VOLUME="/Volumes/XIAO-SENSE"
PORT=""
BOARD="xiao_ble/nrf52840/sense"

NO_BUILD=false
NO_MONITOR=false

for arg in "$@"; do
    case $arg in
        --no-build)   NO_BUILD=true ;;
        --no-monitor) NO_MONITOR=true ;;
    esac
done

# ── Fonctions ──────────────────────────────────────────────

find_port() {
    PORT=$(ls /dev/tty.usbmodem* 2>/dev/null | head -1)
    if [ -z "$PORT" ]; then
        echo "❌ Aucun port série trouvé. Branche le board !"
        exit 1
    fi
    echo "📡 Port série: $PORT"
}

kill_screen() {
    local pid
    pid=$(lsof -t "$PORT" 2>/dev/null || true)
    if [ -n "$pid" ]; then
        echo "🔪 Fermeture de screen (PID $pid)..."
        kill -9 "$pid" 2>/dev/null || true
        sleep 1
    fi
}

enter_bootloader() {
    echo "🔄 Reset en mode bootloader (1200 baud touch)..."
    stty -f "$PORT" 1200 2>/dev/null || true
    
    echo "⏳ Attente du volume $VOLUME..."
    for i in $(seq 1 15); do
        if [ -d "$VOLUME" ]; then
            echo "✅ Bootloader détecté !"
            return 0
        fi
        sleep 1
    done
    
    echo "❌ Timeout: $VOLUME non monté. Essaie le double-tap reset."
    exit 1
}

do_build() {
    echo "🔨 Build du firmware..."
    source "$ZEPHYR_DIR/.venv/bin/activate"
    export ZEPHYR_BASE="$ZEPHYR_DIR/zephyr"
    export ZEPHYR_SDK_INSTALL_DIR="$HOME/zephyr-sdk-1.0.1"
    
    cd "$ZEPHYR_DIR"
    west build -p always -b "$BOARD" "$FIRMWARE_DIR" 2>&1
    
    if [ ! -f "$UF2_FILE" ]; then
        echo "❌ Build échoué: $UF2_FILE introuvable"
        exit 1
    fi
    echo "✅ Build OK: $(du -h "$UF2_FILE" | cut -f1)"
}

do_flash() {
    echo "⚡ Flash: copie de zephyr.uf2..."
    cp "$UF2_FILE" "$VOLUME/" 2>/dev/null || true
    
    echo "⏳ Attente du reboot..."
    for i in $(seq 1 10); do
        if [ ! -d "$VOLUME" ]; then
            echo "✅ Flash réussi, board redémarré !"
            return 0
        fi
        sleep 1
    done
    echo "⚠️  Le volume est encore là, vérifie le board."
}

do_monitor() {
    sleep 3
    find_port
    echo "📺 Monitor série sur $PORT (Ctrl+A K Y pour quitter)"
    screen "$PORT" 115200
}

# ── Main ───────────────────────────────────────────────────

echo "╔══════════════════════════════════╗"
echo "║     AntiCant Flash Tool          ║"
echo "╚══════════════════════════════════╝"

# Build
if [ "$NO_BUILD" = false ]; then
    do_build
else
    echo "⏭️  Build skippé (--no-build)"
fi

# Flash
find_port
kill_screen
enter_bootloader
do_flash

# Monitor
if [ "$NO_MONITOR" = false ]; then
    do_monitor
else
    echo "⏭️  Monitor skippé (--no-monitor)"
fi
