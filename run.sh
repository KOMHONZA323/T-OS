#!/bin/bash

# Configuration
BUILD_DIR="build"
IMAGE_FILE="$BUILD_DIR/tos.img"
OVMF_DIR="ovmf"
OVMF_CODE="$OVMF_DIR/OVMF_CODE.fd"
OVMF_VARS="$OVMF_DIR/OVMF_VARS.fd"

# Ensure build directory exists
if [ ! -f "$IMAGE_FILE" ]; then
    echo "Error: T-OS image not found at $IMAGE_FILE"
    echo "Please build the project first:"
    echo "  mkdir -p $BUILD_DIR"
    echo "  cd $BUILD_DIR"
    echo "  cmake .."
    echo "  make"
    echo "  make tos_image"
    exit 1
fi

# Setup local OVMF if not present
if [ ! -d "$OVMF_DIR" ]; then
    echo "Setting up local OVMF..."
    mkdir -p "$OVMF_DIR"

    # Try to find OVMF on the system
    # Fedora path
    if [ -f "/usr/share/edk2/ovmf/OVMF_CODE.fd" ]; then
        cp /usr/share/edk2/ovmf/OVMF_CODE.fd "$OVMF_CODE"
        cp /usr/share/edk2/ovmf/OVMF_VARS.fd "$OVMF_VARS"
    # Debian/Ubuntu path
    elif [ -f "/usr/share/ovmf/OVMF.fd" ]; then
        cp /usr/share/ovmf/OVMF.fd "$OVMF_CODE"
        cp /usr/share/ovmf/OVMF_VARS.fd "$OVMF_VARS" # Sometimes it's the same file or split
        # If VARS doesn't exist separately on Ubuntu (older versions), just use the main FD for both (not ideal but works)
        if [ ! -f "$OVMF_VARS" ]; then
             cp /usr/share/ovmf/OVMF.fd "$OVMF_VARS"
        fi
    else
        echo "Error: Could not find OVMF firmware on your system."
        echo "Please install 'edk2-ovmf' (Fedora) or 'ovmf' (Debian/Ubuntu)."
        exit 1
    fi

    # Ensure we have write permissions on our local copy of VARS
    chmod +w "$OVMF_VARS"
fi

echo "Starting T-OS in QEMU..."
qemu-system-x86_64 \
    -drive if=pflash,format=raw,readonly=on,file="$OVMF_CODE" \
    -drive if=pflash,format=raw,file="$OVMF_VARS" \
    -drive format=raw,file="$IMAGE_FILE" \
    -net none \
    -serial stdio
