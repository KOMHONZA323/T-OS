#!/bin/bash
set -e

# Create build directory
mkdir -p build/uefi

echo "Compiling..."
# Compile
gcc -ffreestanding -m64 -fno-stack-protector -fpic -fshort-wchar -mno-red-zone -c src/uefi/main.c -o build/uefi/main.o

echo "Linking..."
# Create shared object
ld -nostdlib -shared -Bsymbolic -e efi_main -o build/uefi/main.so build/uefi/main.o

echo "Creating EFI Executable..."
# Convert to EFI executable
objcopy --target=pei-x86-64 --subsystem=10 build/uefi/main.so build/uefi/BOOTX64.EFI

# Create disk image structure
mkdir -p build/uefi_image/EFI/BOOT
cp build/uefi/BOOTX64.EFI build/uefi_image/EFI/BOOT/BOOTX64.EFI

echo "Build complete: build/uefi_image/EFI/BOOT/BOOTX64.EFI"
echo "To run in QEMU (requires OVMF):"
echo "qemu-system-x86_64 -bios path/to/OVMF.fd -drive file=fat:rw:build/uefi_image,format=raw"
