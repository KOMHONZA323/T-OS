#!/bin/bash
set -e

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Configuration
CC=gcc
OBJCOPY=objcopy
CFLAGS="-m64 -ffreestanding -fno-stack-protector -fno-stack-check -fshort-wchar -mno-red-zone -maccumulate-outgoing-args -Wall -Wextra"
LDFLAGS="-shared -Bsymbolic -nostdlib"

# Output
SRC="${DIR}/hello.c"
OBJ="${DIR}/hello.o"
SO="${DIR}/hello.so"
EFI="${DIR}/BOOTX64.EFI"

echo "Compiling ${SRC}..."
${CC} ${CFLAGS} -c ${SRC} -o ${OBJ}

echo "Linking ${OBJ}..."
${CC} ${LDFLAGS} ${OBJ} -o ${SO}

echo "Converting to UEFI executable..."
# Note: --target=efi-app-x86_64 is standard for converting ELF to UEFI PE
${OBJCOPY} -j .text -j .sdata -j .data -j .dynamic -j .dynsym -j .rel -j .rela -j .reloc --target=efi-app-x86_64 ${SO} ${EFI}

echo "Build complete: ${EFI}"
ls -l ${EFI}
