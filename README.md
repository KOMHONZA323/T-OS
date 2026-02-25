# T-OS: A Self-Hosted x86_64 UEFI Operating System

T-OS is a from-scratch, bare-metal operating system built for x86_64 architectures using UEFI. It features a custom hybrid toolchain (TGC) supporting C and HolyC, a unique UI blending Fedora aesthetics with Windows 7 functionality, and a focus on self-hosting.

## Prerequisites

*   **Host OS:** Fedora Linux (recommended) or other Linux distributions.
*   **Build Tools:**
    *   `cmake`
    *   `make` (or `ninja`)
    *   `gcc` (x86_64-elf cross-compiler or standard gcc with freestanding flags)
    *   `python3` (for image generation)
*   **Emulation:**
    *   `qemu-system-x86_64`
    *   `OVMF` (UEFI firmware for QEMU)

## Build Instructions

1.  **Clone the Repository:**
    ```bash
    git clone <repository-url>
    cd T-OS
    ```

2.  **Create Build Directory:**
    ```bash
    mkdir build
    cd build
    ```

3.  **Configure with CMake:**
    ```bash
    cmake ..
    ```

4.  **Build the Project:**
    ```bash
    make
    ```

5.  **Generate Disk Image:**
    This step creates the `tos.img` partitioned disk image containing the EFI bootloader.
    ```bash
    make tos_image
    ```

## Running with QEMU

To run T-OS in QEMU, you need the OVMF UEFI firmware (`edk2-ovmf` on Fedora, `ovmf` on Debian/Ubuntu).

1.  **Run the helper script:**
    Execute the following command from the project root:

    ```bash
    ./run.sh
    ```

    This script automatically locates the system's OVMF firmware, copies it to a local `ovmf/` directory to avoid permission issues, and launches QEMU.

## Flashing to USB (Real Hardware)

To run T-OS on real hardware, you need to write the generated disk image (`tos.img`) to a USB flash drive.
**WARNING: This process will erase all data on the USB drive.**

1.  **Build the disk image:**
    Follow the build instructions above to generate `build/tos.img`.

2.  **Insert your USB drive.**

3.  **Identify the USB drive:**
    Run `lsblk` to list block devices. Identify your USB drive (e.g., `/dev/sdb` or `/dev/sdc`).
    Make sure it is NOT your system drive (usually `/dev/sda` or `/dev/nvme0n1`).

4.  **Unmount the drive:**
    If the drive was automatically mounted, unmount all partitions:
    ```bash
    sudo umount /dev/sdX*
    ```
    (Replace `/dev/sdX` with your drive identifier).

5.  **Write the image:**
    Use `dd` to write the image to the USB drive:
    ```bash
    sudo dd if=build/tos.img of=/dev/sdX bs=4M status=progress && sync
    ```

6.  **Boot:**
    Insert the USB drive into your target machine, boot into the UEFI BIOS settings, and select the USB drive as the boot device.

## Directory Structure

*   `boot/`: UEFI Bootloader source (Freestanding C).
*   `kernel/`: Core kernel logic (T-Exec).
*   `drivers/`: Hardware drivers (T-HAL).
*   `gui/`: Windowing system and UI components.
*   `toolchain/`: Host-side tools (TGC compiler, T-Link).
*   `apps/`: User-space applications.
*   `cmake/`: CMake toolchain configurations.
*   `scripts/`: Utility scripts for image generation.
