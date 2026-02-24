#!/bin/bash
set -e

# Define paths
UEFI_BUILD="src/uefi/main.efi"
DISK_IMG="uefi.img"
OVMF_PATH="/usr/share/ovmf/OVMF.fd"

# Check if build artifact exists
if [ ! -f "$UEFI_BUILD" ]; then
    echo "Error: $UEFI_BUILD not found. Please run src/uefi/build.sh first."
    exit 1
fi

echo "Creating disk image..."
dd if=/dev/zero of="$DISK_IMG" bs=1k count=1440 2>/dev/null
mkfs.fat -F 12 "$DISK_IMG" >/dev/null

# Create directories in image
mmd -i "$DISK_IMG" ::/EFI
mmd -i "$DISK_IMG" ::/EFI/BOOT

# Copy EFI application
mcopy -i "$DISK_IMG" "$UEFI_BUILD" ::/EFI/BOOT/BOOTX64.EFI

echo "Starting QEMU..."
# Run QEMU with OVMF firmware
# -bios: specify the OVMF firmware
# -net none: disable network (faster startup, no noise)
# -drive: mount the image as a raw disk
# Use -nographic to run in terminal if no GUI is available, but for graphical output (GOP), we might need VNC or similar.
# The user prompt implies viewing the output ("screenshot", "tri-brid style").
# In this environment, we can't see the GUI directly unless we setup VNC or capture output.
# I'll default to standard run, but user can add -nographic if needed.
qemu-system-x86_64 -bios "$OVMF_PATH" -net none -drive format=raw,file="$DISK_IMG"
