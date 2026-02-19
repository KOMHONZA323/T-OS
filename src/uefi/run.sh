#!/bin/bash
set -e

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Configuration
EFI="${DIR}/BOOTX64.EFI"
IMAGE_DIR="${DIR}/image"
BOOT_DIR="${IMAGE_DIR}/EFI/BOOT"

# Attempt to find OVMF
OVMF_PATH=""
POSSIBLE_PATHS=(
    "/usr/share/ovmf/OVMF.fd"
    "/usr/share/qemu/OVMF.fd"
    "/usr/share/OVMF/OVMF_CODE.fd"
    "/usr/share/EDK2/ovmf/OVMF.fd"
)

for path in "${POSSIBLE_PATHS[@]}"; do
    if [ -f "$path" ]; then
        OVMF_PATH="$path"
        break
    fi
done

if [ -z "$OVMF_PATH" ]; then
    echo "Warning: OVMF firmware not found in standard locations."
    echo "Please ensure 'ovmf' is installed."
    echo "Falling back to 'OVMF.fd' in current directory (if exists)."
    OVMF_PATH="OVMF.fd"
fi

if [ ! -f "$EFI" ]; then
    echo "Error: $EFI not found. Please run compile.sh first."
    exit 1
fi

# Setup directory structure for UEFI Boot
# UEFI looks for /EFI/BOOT/BOOTX64.EFI on the boot media
rm -rf "$IMAGE_DIR"
mkdir -p "$BOOT_DIR"
cp "$EFI" "$BOOT_DIR/BOOTX64.EFI"

echo "Launching QEMU with UEFI..."
echo "Press Ctrl+Alt+G to release mouse capture if needed."

# Run QEMU
# -bios: Use the OVMF firmware
# -drive file=fat:rw:image: Mount the 'image' directory as a FAT drive (HDD)
# -net none: Disable networking
qemu-system-x86_64 \
    -bios "$OVMF_PATH" \
    -drive file=fat:rw:"$IMAGE_DIR",media=disk,format=raw \
    -net none
