#!/bin/bash
# run_graphical_test.sh
# Builds T-OS and runs it with a display for interactive testing.

set -e

echo "--- Step 1: Building the OS Image ---"
mkdir -p build
(cd build && cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain-x86_64.cmake .. && make image)
echo "Build complete."

echo
echo "--- Step 2: Preparing and Running QEMU with Display ---"
# Find firmware
OVMF_CODE_PATH=""
declare -a FW_PATHS=("/usr/share/OVMF" "/usr/share/ovmf" "/usr/share/edk2-ovmf/x64" "/usr/share/edk2/ovmf")
for path in "${FW_PATHS[@]}"; do
    if [ -f "$path/OVMF_CODE.fd" ]; then
        OVMF_CODE_PATH="$path/OVMF_CODE.fd"
        break
    fi
done

if [ -z "$OVMF_CODE_PATH" ]; then
    echo "ERROR: Could not find 'OVMF_CODE.fd'. Please install 'ovmf' or 'edk2-ovmf'."
    exit 1
fi

echo "Found UEFI firmware: $OVMF_CODE_PATH"
echo "Starting QEMU with display. Close the QEMU window to exit."

# Assemble QEMU command
QEMU_CMD=(
    "qemu-system-x86_64"
    "-m" "256M"
    "-net" "none"
    "-drive" "file=build/tos.img,format=raw"
    "-drive" "if=pflash,format=raw,readonly=on,file=$OVMF_CODE_PATH"
    "-serial" "stdio"
)

# Run QEMU
"${QEMU_CMD[@]}"
