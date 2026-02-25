#!/bin/bash
set -e

# Compile
./build.sh

# Create Disk Image
echo "Creating FAT12 disk image..."
python3 make_uefi_image.py main.efi uefi.img

echo "Image created: uefi.img"

# Instructions for running
echo ""
echo "To run this in QEMU, you need 'ovmf' or similar UEFI firmware installed."
echo "Command example:"
echo "qemu-system-x86_64 -bios /usr/share/ovmf/OVMF.fd -drive file=uefi.img,format=raw"
