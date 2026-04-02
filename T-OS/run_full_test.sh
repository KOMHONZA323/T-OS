#!/bin/bash
# run_full_test.sh
# Builds T-OS, runs it headlessly in QEMU, captures serial output, and verifies boot.

set -e # Exit immediately if a command exits with a non-zero status.

echo "--- Step 1: Building the OS Image ---"
# Ensure the build directory exists and run make
mkdir -p build
(cd build && cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain-x86_64.cmake .. > /dev/null && make image)
echo "Build complete."
echo "Image 'build/tos.img' is ready."

echo
echo "--- Step 2: Locating UEFI Firmware ---"
# Find the necessary OVMF firmware files
OVMF_CODE_PATH=""
OVMF_VARS_PATH=""
declare -a FW_PATHS=("/usr/share/OVMF" "/usr/share/ovmf" "/usr/share/edk2-ovmf/x64" "/usr/share/edk2/ovmf")

for path in "${FW_PATHS[@]}"; do
    if [ -f "$path/OVMF_CODE.fd" ]; then
        OVMF_CODE_PATH="$path/OVMF_CODE.fd"
        [ -f "$path/OVMF_VARS.fd" ] && OVMF_VARS_PATH="$path/OVMF_VARS.fd"
        break
    fi
done

if [ -z "$OVMF_CODE_PATH" ]; then
    echo "ERROR: Could not find 'OVMF_CODE.fd'. Please install 'ovmf' or 'edk2-ovmf'."
    exit 1
fi
echo "Found UEFI firmware: $OVMF_CODE_PATH"

echo
echo "--- Step 3: Preparing and Running QEMU Headless for 10 seconds ---"
# Create a temporary, writable copy of the UEFI variables for QEMU to use
TEMP_VARS_FILE=""
if [ -n "$OVMF_VARS_PATH" ]; then
    TEMP_VARS_FILE=$(mktemp)
    cp "$OVMF_VARS_PATH" "$TEMP_VARS_FILE"
fi

# Assemble the full QEMU command
QEMU_CMD=(
    "qemu-system-x86_64"
    "-m" "512M"
    "-net" "none"
    "-device" "virtio-gpu-pci"
    "-drive" "file=build/tos.img,format=raw"
    "-drive" "if=pflash,format=raw,readonly=on,file=$OVMF_CODE_PATH"
    "-display" "none"
    "-serial" "file:serial.log" # IMPORTANT: Output is logged to serial.log
)
if [ -n "$TEMP_VARS_FILE" ]; then
    QEMU_CMD+=("-drive" "if=pflash,format=raw,file=$TEMP_VARS_FILE")
fi

# Run QEMU in the background and stop it after a delay
echo "Starting QEMU... (output will be in 'serial.log')"
"${QEMU_CMD[@]}" &
QEMU_PID=$!
sleep 10
kill $QEMU_PID
wait $QEMU_PID 2>/dev/null || true # Wait for QEMU to terminate, suppress error if it's already gone
echo "QEMU has finished."

# Clean up the temporary file
if [ -n "$TEMP_VARS_FILE" ]; then
    rm "$TEMP_VARS_FILE"
fi

echo
echo "--- Step 4: Verifying the Output ---"
if [ -f "serial.log" ]; then
    echo "Contents of 'serial.log':"
    echo "-------------------------------------"
    cat serial.log
    echo "-------------------------------------"
    if grep -q "VERSION 0.1 ALPHA A" serial.log; then
        echo
        echo "SUCCESS: T-OS v0.1 Alpha A booted successfully."
    else
        echo
        echo "FAILURE: The version string was NOT found."
    fi
else
    echo
    echo "FAILURE: 'serial.log' was not created. QEMU failed to start or write the log."
fi
