#!/bin/bash
set -e

# Configuration
IMAGE="tos.img"
# The bootloader is built by CMake in build/boot/bootx64.efi usually, or boot/bootx64.efi if in source
# Since this script runs from build directory via make, we expect the file relative to CMAKE_BINARY_DIR
# But we are running from the source root usually? No, "COMMAND ${CMAKE_SOURCE_DIR}/scripts/build_image.sh"
# CMake custom target runs in CMAKE_BINARY_DIR.
EFI_FILE="boot/bootx64.efi"
PART_SIZE_MB=64

# Ensure EFI file exists
if [ ! -f "$EFI_FILE" ]; then
    echo "Error: $EFI_FILE not found. Please build the project first."
    exit 1
fi

echo "Creating FAT32 Partition Image..."
# Create a blank file of size 64MB
dd if=/dev/zero of=part.img bs=1M count=$PART_SIZE_MB 2>/dev/null

# Format as FAT32
mkfs.fat -F 32 -n "T-OS" part.img >/dev/null

# Create directory structure
mmd -i part.img ::/EFI
mmd -i part.img ::/EFI/BOOT

# Copy the bootloader
mcopy -i part.img $EFI_FILE ::/EFI/BOOT/BOOTX64.EFI

echo "Creating Disk Image ($IMAGE)..."
# Create the full disk image (Partition size + 2MB for GPT headers/backup)
dd if=/dev/zero of=$IMAGE bs=1M count=$((PART_SIZE_MB + 2)) 2>/dev/null

# Create GPT partition table
parted -s $IMAGE mklabel gpt
parted -s $IMAGE mkpart primary fat32 2048s 100%
parted -s $IMAGE set 1 esp on

# Copy the partition image into the disk image at offset 1MB (2048 sectors * 512 bytes)
dd if=part.img of=$IMAGE bs=1M seek=1 conv=notrunc 2>/dev/null

# Cleanup
rm part.img

echo "T-OS Image ($IMAGE) created successfully."
