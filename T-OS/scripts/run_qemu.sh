#!/bin/bash
# T-OS UEFI QEMU Runner (Fixed for compatibility)

set -e # Exit immediately if a command exits with a non-zero status.

# --- Find Project Root and Image ---
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
IMAGE_PATH="$ROOT_DIR/build/tos.img"

if [ ! -f "$IMAGE_PATH" ]; then
    echo "Error: Image not found at '$IMAGE_PATH'."
    echo "Please run 'make image' in the 'build' directory first."
    exit 1
fi

# --- Find UEFI Firmware (OVMF) ---
OVMF_CODE_PATH=""
OVMF_VARS_PATH=""

# Common paths for OVMF firmware files
declare -a FW_PATHS=(
    "/usr/share/OVMF"
    "/usr/share/ovmf"
    "/usr/share/edk2-ovmf/x64"
    "/usr/share/edk2/ovmf"
)

for path in "${FW_PATHS[@]}"; do
    if [ -f "$path/OVMF_CODE.fd" ]; then
        OVMF_CODE_PATH="$path/OVMF_CODE.fd"
        # The corresponding VARS file is often in the same directory
        if [ -f "$path/OVMF_VARS.fd" ]; then
            OVMF_VARS_PATH="$path/OVMF_VARS.fd"
        fi
        break
    fi
done

if [ -z "$OVMF_CODE_PATH" ]; then
    echo "Error: Could not find 'OVMF_CODE.fd'. This is required for UEFI boot."
    echo "Please install 'ovmf' (Debian/Ubuntu) or 'edk2-ovmf' (Fedora/Arch)."
    exit 1
fi

if [ -z "$OVMF_VARS_PATH" ]; then
    echo "Warning: Could not find 'OVMF_VARS.fd'. Booting may be unreliable."
    # We can proceed without it, but it's not ideal.
fi

# --- Prepare UEFI Variables ---
# QEMU needs a writable copy of the VARS file.
# We create a temporary one for this session.
TEMP_VARS_FILE=""
if [ -n "$OVMF_VARS_PATH" ]; then
    TEMP_VARS_FILE=$(mktemp)
    cp "$OVMF_VARS_PATH" "$TEMP_VARS_FILE"
fi

# --- Assemble QEMU command ---
QEMU_CMD=(
    "qemu-system-x86_64"
    "-m" "512M"
    "-net" "none"
    "-device" "virtio-gpu-pci"
    "-drive" "file=$IMAGE_PATH,format=raw"
    "-device usb-tablet"
)

# Use the more robust -pflash method for UEFI
QEMU_CMD+=("-drive" "if=pflash,format=raw,readonly=on,file=$OVMF_CODE_PATH")
if [ -n "$TEMP_VARS_FILE" ]; then
    QEMU_CMD+=("-drive" "if=pflash,format=raw,file=$TEMP_VARS_FILE")
fi

# Handle headless mode
if [[ "$*" == *"--headless"* ]]; then
    QEMU_CMD+=("-display" "none" "-serial" "stdio")
    echo "Running in HEADLESS mode. Use Ctrl+A, X to exit QEMU."
else
    QEMU_CMD+=("-serial" "stdio")
    echo "Running with display. Check terminal for serial output. Close window to exit."
fi

# --- Launch ---
echo "Using UEFI Firmware: $OVMF_CODE_PATH"
echo "Launching T-OS..."
"${QEMU_CMD[@]}"

# Cleanup
if [ -n "$TEMP_VARS_FILE" ]; then
    rm "$TEMP_VARS_FILE"
fi
