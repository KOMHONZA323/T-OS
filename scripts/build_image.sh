#!/bin/bash

# Build directory passed as first argument
BUILD_DIR=$1
EFI_FILE="$BUILD_DIR/boot/bootx64.efi"
IMAGE_FILE="$BUILD_DIR/tos.img"
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

if [ -z "$BUILD_DIR" ]; then
    echo "Usage: build_image.sh <build_directory>"
    exit 1
fi

if [ ! -f "$EFI_FILE" ]; then
    echo "Error: EFI file not found at $EFI_FILE"
    exit 1
fi

echo "Creating FAT16 disk image..."
# Use the Python script to create the image and inject the EFI file
python3 "$SCRIPT_DIR/make_fat.py" "$IMAGE_FILE" "$EFI_FILE"

if [ $? -eq 0 ]; then
    echo "T-OS disk image created at $IMAGE_FILE"
else
    echo "Failed to create disk image."
    exit 1
fi
