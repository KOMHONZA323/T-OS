#!/bin/bash
set -e

# Setup directory structure if running standalone
# (Assuming the files are placed correctly, but this shows the compile commands)

echo "Compiling Compositor and Kernel..."
# Compile compositor and kernel
x86_64-elf-gcc -ffreestanding -mno-red-zone -m64 -c compositor.c -o compositor.o
x86_64-elf-gcc -ffreestanding -mno-red-zone -m64 -c kernel.c -o kernel.o

# Link them (Assuming entry.o exists and linker script is appropriate)
# x86_64-elf-ld -nostdlib -Ttext 0x1000000 kernel.o compositor.o -o kernel.elf

echo "For the CMake build system, run:"
echo "mkdir -p build && cd build"
echo "cmake .."
echo "make image"
echo ""
echo "Headless QEMU Test Command:"
echo "qemu-system-x86_64 -display none -vga std -drive if=pflash,format=raw,readonly=on,file=/usr/share/OVMF/OVMF_CODE.fd -drive file=build/tos.img,format=raw -serial stdio"
