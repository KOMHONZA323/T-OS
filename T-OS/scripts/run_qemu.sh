#!/bin/bash
# T-OS UEFI QEMU Runner
#
#   scripts/run_qemu.sh              graphical window, serial on stdio
#   scripts/run_qemu.sh --headless   no window, serial on stdio
#   scripts/run_qemu.sh --headless --timeout 30 --serial-log out.txt
#   scripts/run_qemu.sh --headless --screenshot shot.ppm --timeout 30
#
# The last form is what CI and the scheduler demo use: run for a fixed number
# of seconds, capture everything the kernel printed on COM1, optionally grab
# the framebuffer, then shut the VM down.

set -e

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
IMAGE_PATH="$ROOT_DIR/build/tos.img"

# shellcheck source=find_ovmf.sh
source "$ROOT_DIR/scripts/find_ovmf.sh"

HEADLESS=0
TIMEOUT=""
SERIAL_LOG=""
SCREENSHOT=""

while [ $# -gt 0 ]; do
    case "$1" in
        --headless)   HEADLESS=1; shift ;;
        --timeout)    TIMEOUT="$2"; shift 2 ;;
        --serial-log) SERIAL_LOG="$2"; shift 2 ;;
        --screenshot) SCREENSHOT="$2"; shift 2 ;;
        *) echo "Unknown option: $1"; exit 2 ;;
    esac
done

if [ ! -f "$IMAGE_PATH" ]; then
    echo "Error: Image not found at '$IMAGE_PATH'."
    echo "Build it first:  mkdir -p build && cd build && cmake .. && make image"
    exit 1
fi

if ! find_ovmf; then
    echo "Error: could not find OVMF firmware (OVMF_CODE.fd / OVMF_CODE_4M.fd)."
    echo "Install 'ovmf' (Debian/Ubuntu) or 'edk2-ovmf' (Fedora/Arch)."
    exit 1
fi

# QEMU needs a writable copy of the variable store.
TEMP_VARS_FILE=""
cleanup() { [ -n "$TEMP_VARS_FILE" ] && rm -f "$TEMP_VARS_FILE"; }
trap cleanup EXIT

QEMU_CMD=(
    qemu-system-x86_64
    -m 512M
    -net none
    -drive "file=$IMAGE_PATH,format=raw"
    -drive "if=pflash,format=raw,readonly=on,file=$OVMF_CODE_PATH"
)

if [ -n "$OVMF_VARS_PATH" ]; then
    TEMP_VARS_FILE=$(mktemp)
    cp "$OVMF_VARS_PATH" "$TEMP_VARS_FILE"
    QEMU_CMD+=(-drive "if=pflash,format=raw,file=$TEMP_VARS_FILE")
else
    echo "Warning: no OVMF_VARS.fd found; booting may be unreliable."
fi

if [ "$HEADLESS" = "1" ]; then
    QEMU_CMD+=(-display none)
else
    QEMU_CMD+=(-device usb-tablet)
fi

# An unattended run drives QEMU over the monitor socket so it can take a
# screenshot and quit cleanly; an interactive one just puts serial on stdio.
if [ -n "$SCREENSHOT" ] || [ -n "$TIMEOUT" ]; then
    MONITOR_SOCK=$(mktemp -u /tmp/tos-monitor.XXXXXX)
    QEMU_CMD+=(-monitor "unix:$MONITOR_SOCK,server,nowait")
    if [ -n "$SERIAL_LOG" ]; then
        QEMU_CMD+=(-serial "file:$SERIAL_LOG")
    else
        QEMU_CMD+=(-serial stdio)
    fi

    echo "Using UEFI firmware: $OVMF_CODE_PATH"
    echo "Launching T-OS (unattended, ${TIMEOUT:-30}s)..."
    "${QEMU_CMD[@]}" &
    QEMU_PID=$!

    # Wait for the monitor socket to appear.
    for _ in $(seq 1 50); do
        [ -S "$MONITOR_SOCK" ] && break
        sleep 0.2
    done

    sleep "${TIMEOUT:-30}"

    if [ -n "$SCREENSHOT" ] && [ -S "$MONITOR_SOCK" ]; then
        printf 'screendump %s\n' "$SCREENSHOT" | timeout 15 socat - "UNIX-CONNECT:$MONITOR_SOCK" >/dev/null 2>&1 || true
        sleep 1
    fi

    if [ -S "$MONITOR_SOCK" ]; then
        printf 'quit\n' | timeout 5 socat - "UNIX-CONNECT:$MONITOR_SOCK" >/dev/null 2>&1 || true
    fi
    wait "$QEMU_PID" 2>/dev/null || true
    rm -f "$MONITOR_SOCK"
    echo "Run finished."
    [ -n "$SERIAL_LOG" ] && echo "Serial output: $SERIAL_LOG"
    [ -n "$SCREENSHOT" ] && echo "Screenshot:    $SCREENSHOT"
    exit 0
fi

QEMU_CMD+=(-serial stdio)
if [ "$HEADLESS" = "1" ]; then
    echo "Running HEADLESS. Use Ctrl+A, X to exit QEMU."
fi
echo "Using UEFI firmware: $OVMF_CODE_PATH"
echo "Launching T-OS..."
exec "${QEMU_CMD[@]}"
