#!/bin/bash
set -e

# Compile the UEFI application
# -shared: Create a shared library (needed for EFI)
# -nostdlib: Do not link against standard libraries
# -m64: Generate 64-bit code
# -fno-stack-protector: Disable stack protection (no runtime support)
# -fpic: Position Independent Code
# -fshort-wchar: Use 16-bit wchar_t (required for UEFI strings)
# -mno-red-zone: Disable Red Zone (UEFI does not support it)
# -e efi_main: Set entry point
gcc -shared -nostdlib -m64 -fno-stack-protector -fpic -fshort-wchar -mno-red-zone -e efi_main -o main.so main.c

# Convert to PE32+ (EFI Executable)
# --target pei-x86-64: Output format for EFI
# --subsystem=10: EFI Application
objcopy --target pei-x86-64 --subsystem=10 main.so main.efi

echo "Build successful! Created main.efi"
