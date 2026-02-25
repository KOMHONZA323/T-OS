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
    This step creates the `tos.img` FAT16 disk image containing the EFI bootloader.
    ```bash
    make tos_image
    ```

## Running with QEMU

To run T-OS in QEMU, you need the OVMF UEFI firmware.

1.  **Locate OVMF:**
    On Fedora: `/usr/share/edk2/ovmf/OVMF_CODE.fd`
    On Ubuntu/Debian: `/usr/share/ovmf/OVMF.fd`

2.  **Run Command:**
    Execute the following command from the `build` directory:

    ```bash
    qemu-system-x86_64 \
        -drive if=pflash,format=raw,readonly=on,file=/usr/share/edk2/ovmf/OVMF_CODE.fd \
        -drive if=pflash,format=raw,file=/usr/share/edk2/ovmf/OVMF_VARS.fd \
        -drive format=raw,file=tos.img \
        -net none
    ```

    *Note: Adjust the OVMF paths according to your distribution.*

## Directory Structure

*   `boot/`: UEFI Bootloader source (Freestanding C).
*   `kernel/`: Core kernel logic (T-Exec).
*   `drivers/`: Hardware drivers (T-HAL).
*   `gui/`: Windowing system and UI components.
*   `toolchain/`: Host-side tools (TGC compiler, T-Link).
*   `apps/`: User-space applications.
*   `cmake/`: CMake toolchain configurations.
*   `scripts/`: Utility scripts for image generation.
