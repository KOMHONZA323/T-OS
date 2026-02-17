# UEFI Hello World Application

This project implements a minimal "Hello World" UEFI application from scratch, without external dependencies like `gnu-efi` or `EDK II`. It demonstrates how to initialize the UEFI System Table, use the Console Output Protocol, and handle basic input.

## Features

- **Freestanding C Implementation**: No standard library dependency.
- **Custom UEFI Headers**: Minimal definitions for System Table and Protocols.
- **PE/COFF Generation**: Uses `gcc` and `objcopy` to create a valid UEFI executable.
- **"Tri-Brid" Aesthetics**: Sets console colors to mimic a vibrant "Luna" / XP style (Bright Blue text on Black background).

## Prerequisites

- `gcc` (with x86_64 support)
- `make`
- `objcopy` (usually part of binutils)
- QEMU (for testing)
- OVMF Firmware image (`OVMF.fd`) for UEFI support in QEMU.

## Compilation

To build the application, simply run `make` in this directory:

```bash
make
```

This will produce `bootx64.efi`.

## Running in QEMU

To run this application, you need to place it in a virtual disk image or directory structure that UEFI recognizes. The standard path for a removable bootloader is `/EFI/BOOT/BOOTX64.EFI`.

### Quick Run (using QEMU VFAT)

1. Create the directory structure:
   ```bash
   mkdir -p image/EFI/BOOT
   cp bootx64.efi image/EFI/BOOT/BOOTX64.EFI
   ```

2. Run QEMU with OVMF:
   ```bash
   # Replace /path/to/OVMF.fd with the actual path to your OVMF firmware file
   # On many Linux distros, it might be at /usr/share/ovmf/OVMF.fd

   qemu-system-x86_64 -bios /path/to/OVMF.fd -net none -drive file=fat:rw:image,media=disk,format=raw
   ```

3. The system should boot into the UEFI shell or directly load your application if it's the only boot option. If it drops to shell, type `exit` or navigate to `fs0:` and run `\EFI\BOOT\BOOTX64.EFI`.

## Code Structure

- `uefi.h`: Minimal UEFI type and protocol definitions.
- `main.c`: The application entry point and logic.
- `linker.ld`: Linker script to organize sections for PE conversion.
- `Makefile`: Build rules.
