#!/bin/bash
set -e

echo "Building T-OS..."
make

echo "Setting up boot directory..."
rm -rf image_dir
mkdir -p image_dir/EFI/BOOT
cp BOOTX64.EFI image_dir/EFI/BOOT/BOOTX64.EFI

echo "Starting T-OS in QEMU..."
echo "(If you don't see a graphical window, QEMU might be running in the background. Close it to stop.)"

qemu-system-x86_64 -machine q35 -m 256M -drive if=pflash,format=raw,readonly=on,file=/usr/share/OVMF/OVMF_CODE.fd -drive format=raw,file=fat:rw:image_dir
