#!/bin/bash
set -e

echo "Compiling UEFI application..."
# Compile to object file
gcc -c -m64 -fno-stack-protector -fpic -fshort-wchar -mno-red-zone -Wall -Wextra -Werror -I src/uefi src/uefi/main.c -o src/uefi/main.o

echo "Linking shared object..."
# Link to shared object
# Use -Bsymbolic to bind references locally
# Use -shared to create a shared object (needed for UEFI relocatable image)
# Use -nostdlib to avoid standard libs
# Use -Wl,-entry:efi_main to set entry point explicitly for the linker
gcc -shared -nostdlib -m64 -Wl,-Bsymbolic -Wl,-e,efi_main src/uefi/main.o -o src/uefi/main.so

echo "Converting to PE executable..."
# Convert to EFI executable
# Copy only necessary sections to avoid garbage
objcopy -j .text -j .sdata -j .data -j .dynamic -j .dynsym -j .rel -j .rela -j .reloc \
        --target pei-x86-64 --subsystem=10 \
        src/uefi/main.so src/uefi/main.efi

echo "Build successful: src/uefi/main.efi"
