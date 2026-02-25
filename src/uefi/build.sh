#!/bin/bash
set -e

echo "Compiling..."
gcc -c main.c -o main.o \
    -std=c11 \
    -Wall \
    -Wextra \
    -fno-stack-protector \
    -fpic \
    -fshort-wchar \
    -mno-red-zone \
    -maccumulate-outgoing-args \
    -ffreestanding \
    -I.

echo "Linking..."
gcc -shared -nostdlib -Wl,-Bsymbolic -Wl,-e,efi_main -o main.so main.o

echo "Converting to EFI..."
objcopy -j .text -j .sdata -j .data -j .dynamic -j .dynsym -j .rel \
        -j .rela -j .rel.* -j .rela.* -j .reloc \
        --target=pei-x86-64 --subsystem=10 \
        main.so main.efi

echo "Done: main.efi created."
