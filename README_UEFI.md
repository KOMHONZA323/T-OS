# Tri-Brid OS (UEFI)

This directory contains the initial work for a 64-bit UEFI-based Operating System.

## Building the UEFI Application

To build the `bootx64.efi` application (Hello World):

1. Ensure you have `gcc` (with 64-bit support) and `objcopy` installed.
2. From the project root, create a build directory:
   ```bash
   mkdir -p build
   cd build
   cmake ..
   cmake --build . --target uefi_hello
   ```
   This will generate `bootx64.efi` in the `build/` directory.

## Running in QEMU

To run the UEFI application in QEMU, you need the OVMF firmware image (`OVMF.fd` or split `OVMF_CODE.fd`/`OVMF_VARS.fd`).

1. **Prepare the Directory Structure:**
   QEMU can boot from a directory simulating a FAT filesystem. Create the standard EFI boot path:
   ```bash
   mkdir -p build/uefi_root/EFI/BOOT
   cp build/bootx64.efi build/uefi_root/EFI/BOOT/BOOTX64.EFI
   ```

2. **Run QEMU:**
   Assuming you have `OVMF.fd` (you might need to install `ovmf` package or download it):
   ```bash
   qemu-system-x86_64 \
       -bios /usr/share/ovmf/OVMF.fd \
       -drive file=fat:rw:build/uefi_root,format=raw,media=disk \
       -net none
   ```
   *Note: Adjust the path to `OVMF.fd` as per your system installation.*

   You should see "Initializing Tri-Brid OS..." on a blue background.
