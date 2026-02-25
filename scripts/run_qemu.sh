#!/bin/bash
# T-OS QEMU Runner

IMAGE="tos.img"
OVMF_CODE="/usr/share/ovmf/OVMF.fd"
OVMF_VARS="OVMF_VARS.fd"

if [ ! -f "$IMAGE" ]; then
    echo "Error: $IMAGE not found. Please build the project first."
    exit 1
fi

if [ ! -f "$OVMF_CODE" ]; then
    echo "Error: OVMF firmware not found."
    exit 1
fi

# Create a local copy of VARS to avoid permission issues
if [ ! -f "$OVMF_VARS" ]; then
    if [ -f "/usr/share/ovmf/OVMF_VARS.fd" ]; then
        cp /usr/share/ovmf/OVMF_VARS.fd $OVMF_VARS
    else
        # Try to find it or create a dummy
        find /usr -name OVMF_VARS.fd -exec cp {} $OVMF_VARS \; -quit
    fi
fi

if [ ! -f "$OVMF_VARS" ]; then
    echo "Warning: OVMF_VARS.fd not found, using dummy (might fail)"
    dd if=/dev/zero of=$OVMF_VARS bs=1M count=4
fi

echo "Starting T-OS in QEMU..."
qemu-system-x86_64 \
    -m 1G \
    -drive if=pflash,format=raw,readonly=on,file=$OVMF_CODE \
    -drive if=pflash,format=raw,file=$OVMF_VARS \
    -drive format=raw,file=$IMAGE \
    -net none \
    -serial stdio
