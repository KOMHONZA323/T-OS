cmake_minimum_required(VERSION 3.10)
project(T-OS ASM_NASM)

# Output Paths
set(BOOT_BIN "${CMAKE_BINARY_DIR}/boot.bin")
set(OS_IMAGE "${CMAKE_BINARY_DIR}/my_os.bin")
set(OS_ISO   "${CMAKE_BINARY_DIR}/T-OS.iso")
set(ISO_ROOT "${CMAKE_BINARY_DIR}/iso_root")

# 1. The Build Target
add_custom_target(t_os ALL
    # Step A: Assemble boot.asm
    COMMAND nasm -f bin ${CMAKE_SOURCE_DIR}/boot.asm -o ${BOOT_BIN}
    
    # Step B: Create 1MB raw binary disk image
    COMMAND dd if=/dev/zero of=${OS_IMAGE} bs=1024 count=1024
    COMMAND dd if=${BOOT_BIN} of=${OS_IMAGE} conv=notrunc
    
    # Step C: Create the ISO
    COMMAND mkdir -p ${ISO_ROOT}/boot
    COMMAND cp ${BOOT_BIN} ${ISO_ROOT}/boot/
    COMMAND xorriso -as mkisofs -b boot/boot.bin -no-emul-boot -boot-load-size 4 -o ${OS_ISO} ${ISO_ROOT}
    
    COMMENT "Building Binaries and ISO in the build folder..."
)

# 2. The RUN Target (The part you asked for)
add_custom_target(run
    COMMAND qemu-system-i386 -cdrom ${OS_ISO}
    DEPENDS t_os
    COMMENT "Launching T-OS in QEMU from the build folder"
)
