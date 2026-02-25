# T-OS: A Custom x86_64 UEFI Operating System

T-OS is an experimental, from-scratch operating system developed in C and Assembly, targeting modern x86_64 UEFI systems. It aims to be self-hosting with a unique toolchain (TGC, HolyC, TEXF).

## Prerequisites

To build T-OS, you need a Linux environment (Fedora/Debian/Ubuntu) with the following packages installed:

*   **Build Tools:** `cmake`, `make`, `gcc`, `binutils`
*   **UEFI Development:** `gnu-efi`
*   **Disk Image Utilities:** `mtools`, `xorriso`, `parted`, `dosfstools`
*   **Emulation:** `qemu-system-x86`, `ovmf` (UEFI firmware)

### Installation (Ubuntu/Debian)
```bash
sudo apt update
sudo apt install -y build-essential cmake gnu-efi qemu-system-x86 ovmf mtools xorriso dosfstools parted
```

### Installation (Fedora)
```bash
sudo dnf install -y cmake make gcc gnu-efi qemu-system-x86 edk2-ovmf mtools xorriso dosfstools parted
```

## Build Instructions

T-OS uses CMake for cross-compilation. The build process generates a bootable disk image (`tos.img`) containing the EFI bootloader.

1.  **Create a Build Directory:**
    ```bash
    mkdir -p build
    cd build
    ```

2.  **Configure CMake (using the x86_64 toolchain):**
    ```bash
    cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain-x86_64.cmake ..
    ```

3.  **Build the Bootloader:**
    ```bash
    make
    ```

4.  **Create the Disk Image:**
    This step packages `bootx64.efi` into a FAT32 partition inside a GPT disk image.
    ```bash
    make image
    ```

## Running in QEMU

To test the OS in an emulator (QEMU with OVMF UEFI firmware):

```bash
# From the build directory
../scripts/run_qemu.sh
```

Ensure you have `ovmf` installed. The script will attempt to locate `OVMF_CODE.fd` and `OVMF_VARS.fd` in standard system paths.

## Flashing to USB (Real Hardware)

**WARNING: This will erase all data on the target USB drive!**

1.  Identify your USB device (e.g., `/dev/sdb`) using `lsblk`.
2.  Unmount any partitions on the device.
3.  Write the image using `dd`:

    ```bash
    sudo dd if=build/tos.img of=/dev/sdX bs=4M status=progress && sync
    ```
    *(Replace `/dev/sdX` with your actual device identifier)*

See `scripts/FLASHIT.txt` for more details.

## Project Structure

*   `boot/`: UEFI Bootloader source code (GNU-EFI).
*   `cmake/`: CMake toolchain files for cross-compilation.
*   `scripts/`: Helper scripts for building images and running QEMU.
*   `kernel/`: (Future) Kernel core.
*   `drivers/`: (Future) Hardware drivers.
*   `gui/`: (Future) Graphics and Windowing system.
*   `apps/`: (Future) User-space applications.
*   `toolchain/`: (Future) Host-compiled bootstrap tools.
