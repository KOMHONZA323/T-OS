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

To run T-OS in QEMU, you need the OVMF UEFI firmware (`edk2-ovmf` on Fedora, `ovmf` on Debian/Ubuntu).

1.  **Run the helper script:**
    Execute the following command from the project root:

    ```bash
    ./run.sh
    ```

    This script automatically locates the system's OVMF firmware, copies it to a local `ovmf/` directory to avoid permission issues, and launches QEMU.

## Directory Structure

*   `boot/`: UEFI Bootloader source (Freestanding C).
*   `kernel/`: Core kernel logic (T-Exec).
*   `drivers/`: Hardware drivers (T-HAL).
*   `gui/`: Windowing system and UI components.
*   `toolchain/`: Host-side tools (TGC compiler, T-Link).
*   `apps/`: User-space applications.
*   `cmake/`: CMake toolchain configurations.
*   `scripts/`: Utility scripts for image generation.
