# T-OS

## Build Instructions

### Prerequisites
- CMake
- NASM
- GCC (configured for 32-bit x86 compilation)
- QEMU (for emulation)

Install dependencies (Ubuntu/Debian):
```bash
sudo apt-get install cmake nasm gcc gcc-multilib qemu-system-x86
```

### Build
```bash
mkdir -p build
cd build
cmake ..
cmake --build .
```

### Run
To run the OS in QEMU:
```bash
cmake --build . --target run
```
