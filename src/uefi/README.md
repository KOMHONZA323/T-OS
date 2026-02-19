# T-OS UEFI Hello World

This directory contains a minimal, freestanding "Hello World" application for UEFI x86_64, built from scratch using GCC and objcopy.

## Prerequisites

To build and run this example, you need a Linux environment with the following tools installed:
- `gcc` (x86_64)
- `binutils` (objcopy)
- `qemu-system-x86_64`
- `ovmf` (UEFI firmware for QEMU)

On Debian/Ubuntu:
```bash
sudo apt update
sudo apt install build-essential qemu-system-x86 ovmf
```

## Compilation

The `compile.sh` script automates the build process:
1.  Compiles `hello.c` into a freestanding object file (`hello.o`).
2.  Links it into a shared object (`hello.so`).
3.  Converts the shared object into a valid PE32+ UEFI executable (`BOOTX64.EFI`) using `objcopy`.

To build:
```bash
./compile.sh
```

## Running in QEMU

To run the application, we need to present `BOOTX64.EFI` to the UEFI firmware in a way it understands (usually on a FAT filesystem at `/EFI/BOOT/BOOTX64.EFI`).

The `run.sh` script handles this by creating a temporary directory structure and launching QEMU with a virtual FAT drive.

To run:
```bash
./run.sh
```

### Manual Execution

If you prefer running manually:
```bash
mkdir -p image/EFI/BOOT
cp BOOTX64.EFI image/EFI/BOOT/BOOTX64.EFI
qemu-system-x86_64 -bios /usr/share/ovmf/OVMF.fd -net none -drive file=fat:rw:image,media=disk,format=raw
```
(Adjust the path to `OVMF.fd` if necessary, e.g., `/usr/share/qemu/OVMF.fd` or `/usr/share/OVMF/OVMF_CODE.fd`).
