# UEFI Hello World

This directory contains a simple UEFI "Hello World" application written in C.

## Build

To compile the application, run the build script:

```bash
./build.sh
```

This will generate `main.efi`.

## Run with QEMU and OVMF

To run the application in QEMU, you need the OVMF firmware (Open Virtual Machine Firmware).

1. **Install OVMF**:
   On Ubuntu/Debian:
   ```bash
   sudo apt-get install ovmf
   ```

2. **Prepare the disk image**:
   Create a directory structure for the EFI bootloader:
   ```bash
   mkdir -p image/EFI/BOOT
   cp main.efi image/EFI/BOOT/BOOTX64.EFI
   ```

3. **Run QEMU**:
   Run QEMU pointing to the directory as a FAT drive and using the OVMF firmware:
   ```bash
   qemu-system-x86_64 -bios /usr/share/ovmf/OVMF.fd -drive file=fat:rw:image,media=disk,format=raw
   ```

   Alternatively, if you prefer using an image file:
   ```bash
   dd if=/dev/zero of=uefi.img bs=1M count=64
   mkfs.fat -F 32 uefi.img
   mmd -i uefi.img ::/EFI
   mmd -i uefi.img ::/EFI/BOOT
   mcopy -i uefi.img main.efi ::/EFI/BOOT/BOOTX64.EFI
   qemu-system-x86_64 -bios /usr/share/ovmf/OVMF.fd -drive file=uefi.img,format=raw
   ```

You should see "Initializing Tri-Brid OS..." on a blue background.
