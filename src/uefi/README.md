# Tri-Brid UEFI OS

This directory contains the source code for the 64-bit UEFI entry point of the Tri-Brid Operating System.

## Prerequisites

- GCC (with x86_64 support)
- GNU Binutils (ld, objcopy)
- QEMU (qemu-system-x86_64)
- OVMF Firmware (OVMF.fd)

## Building

To build the UEFI application `BOOTX64.EFI`, run the build script:

```bash
./src/uefi/build.sh
```

This will generate `build/uefi_image/EFI/BOOT/BOOTX64.EFI`.

## Running in QEMU

You need the OVMF firmware image (`OVMF.fd`). On Debian/Ubuntu, you can install it via `sudo apt install ovmf` and find it at `/usr/share/ovmf/OVMF.fd`.

Run the following command:

```bash
qemu-system-x86_64 -bios /usr/share/ovmf/OVMF.fd -drive file=fat:rw:build/uefi_image,format=raw
```

This mounts the `build/uefi_image` directory as a FAT drive, which UEFI will detect as a bootable partition.
