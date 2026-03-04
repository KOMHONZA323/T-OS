#!/bin/bash
# T-OS USB Flasher script

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
IMAGE="$ROOT_DIR/build/tos.img"

if [ "$#" -ne 1 ]; then
    echo "Usage: sudo $0 /dev/sdX"
    echo "  Example: sudo $0 /dev/sdb"
    echo "  Check available drives with: lsblk"
    exit 1
fi

DEVICE="$1"

# Check if the image exists
if [ ! -f "$IMAGE" ]; then
    echo "Error: $IMAGE not found! Run 'make image' in the build directory."
    exit 1
fi

# Check if the device exists and is a block device
if [ ! -b "$DEVICE" ]; then
    echo "Error: $DEVICE is not a valid block device!"
    exit 1
fi

# Check if the user is root
if [ "$EUID" -ne 0 ]; then
    echo "Please run this script as root (use sudo)."
    exit 1
fi

# Ask for confirmation
echo "--------------------------------------------------------"
echo "WARNING: You are about to flash T-OS to $DEVICE"
echo "ALL DATA ON $DEVICE WILL BE PERMANENTLY DELETED!"
echo "--------------------------------------------------------"
read -p "Are you absolutely sure you want to proceed? (type 'yes' to confirm): " CONFIRM

if [ "$CONFIRM" != "yes" ]; then
    echo "Aborting. No changes were made."
    exit 1
fi

echo "Flashing T-OS image to $DEVICE..."
dd if="$IMAGE" of="$DEVICE" bs=4M status=progress conv=fsync

if [ $? -eq 0 ]; then
    echo "Successfully flashed T-OS to $DEVICE!"
    echo "You can now boot from this USB on any UEFI-capable x86_64 machine."
else
    echo "Error: Failed to flash the image."
    exit 1
fi
