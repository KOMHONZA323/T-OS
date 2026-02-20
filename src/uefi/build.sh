#!/bin/bash
set -e

# Ensure we are in the project root or adjust paths
if [ -f "src/uefi/main.c" ]; then
    BASE_DIR="."
elif [ -f "main.c" ]; then
    BASE_DIR=".."
    cd src/uefi
else
    echo "Error: Could not find source files. Run from project root."
    exit 1
fi

UEFI_DIR="src/uefi"
BUILD_DIR="src/uefi"

# Compilation
echo "Compiling UEFI Application..."
gcc -I${UEFI_DIR} -ffreestanding -mno-red-zone -fno-stack-protector -fpic -fshort-wchar -m64 -c ${UEFI_DIR}/main.c -o ${BUILD_DIR}/main.o

# Linking
echo "Linking..."
ld -shared -Bsymbolic -o ${BUILD_DIR}/main.so ${BUILD_DIR}/main.o

# Convert to PE32+ (UEFI Executable)
echo "Converting to EFI Executable..."
objcopy -j .text -j .sdata -j .data -j .dynamic -j .dynsym -j .rel -j .rela -j .reloc --target=pei-x86-64 --subsystem=10 ${BUILD_DIR}/main.so ${BUILD_DIR}/main.efi

echo "Build Complete: ${BUILD_DIR}/main.efi"

# File Check
file ${BUILD_DIR}/main.efi

# Run instructions
echo ""
echo "To run in QEMU (requires OVMF):"
echo "1. Create directory structure:"
echo "   mkdir -p efi_img/EFI/BOOT"
echo "2. Copy executable:"
echo "   cp ${BUILD_DIR}/main.efi efi_img/EFI/BOOT/BOOTX64.EFI"
echo "3. Run QEMU:"
echo "   qemu-system-x86_64 -drive if=pflash,format=raw,readonly=on,file=/usr/share/ovmf/OVMF.fd -drive format=raw,file=fat:rw:efi_img -net none"
