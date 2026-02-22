#!/bin/bash
set -e

# Compile the UEFI application
# -shared: Create a shared object
# -nostdlib: Do not link standard libraries
# -m64: Generate 64-bit code
# -fno-stack-protector: Disable stack protection
# -fpic: Position Independent Code
# -fshort-wchar: Use 16-bit wchar_t (required for UEFI)
# -mno-red-zone: Disable red zone (required for UEFI/kernel)
# -e efi_main: Set entry point
gcc -shared -nostdlib -m64 -fno-stack-protector -fpic -fshort-wchar -mno-red-zone -e efi_main -o src/uefi/main.so src/uefi/main.c

# Convert to UEFI PE32+ executable
# --target pei-x86-64: Target format for UEFI
# --subsystem=10: EFI Application subsystem
objcopy --target pei-x86-64 --subsystem=10 src/uefi/main.so src/uefi/main.efi

echo "Build complete: src/uefi/main.efi"
