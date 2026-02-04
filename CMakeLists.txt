cmake_minimum_required(VERSION 3.10)
project(T-OS C ASM_NASM)

# Compiler Flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffreestanding -m32 -g -fno-pie -fno-stack-protector")

# Paths
set(BUILD_DIR "${CMAKE_BINARY_DIR}")
set(BOOT_SRC "${CMAKE_SOURCE_DIR}/src/boot/boot.asm")
set(KERNEL_ENTRY "${CMAKE_SOURCE_DIR}/src/kernel/kernel_entry.asm")
set(KERNEL_C "${CMAKE_SOURCE_DIR}/src/kernel/kernel.c")

set(BOOT_BIN "${BUILD_DIR}/boot.bin")
set(KERNEL_ENTRY_OBJ "${BUILD_DIR}/kernel_entry.o")
set(KERNEL_OBJ "${BUILD_DIR}/kernel.o")
set(KERNEL_BIN "${BUILD_DIR}/kernel.bin")
set(OS_IMAGE "${BUILD_DIR}/t-os.bin")

# 1. Compile Bootloader
add_custom_command(OUTPUT ${BOOT_BIN}
    COMMAND nasm -f bin ${BOOT_SRC} -o ${BOOT_BIN} -I ${CMAKE_SOURCE_DIR}/
    DEPENDS ${BOOT_SRC} "${CMAKE_SOURCE_DIR}/src/boot/disk_load.asm" "${CMAKE_SOURCE_DIR}/src/boot/gdt.asm" "${CMAKE_SOURCE_DIR}/src/boot/print_string.asm" "${CMAKE_SOURCE_DIR}/src/boot/print_string_pm.asm" "${CMAKE_SOURCE_DIR}/src/boot/switch_to_pm.asm"
    COMMENT "Assembling Bootloader"
)

# 2. Assemble Kernel Entry
add_custom_command(OUTPUT ${KERNEL_ENTRY_OBJ}
    COMMAND nasm -f elf ${KERNEL_ENTRY} -o ${KERNEL_ENTRY_OBJ}
    DEPENDS ${KERNEL_ENTRY}
    COMMENT "Assembling Kernel Entry"
)

# 3. Compile Kernel C
add_custom_command(OUTPUT ${KERNEL_OBJ}
    COMMAND gcc -ffreestanding -m32 -g -fno-pie -fno-pic -fno-stack-protector -c ${KERNEL_C} -o ${KERNEL_OBJ}
    DEPENDS ${KERNEL_C}
    COMMENT "Compiling Kernel C"
)

# 4. Link Kernel
add_custom_command(OUTPUT ${KERNEL_BIN}
    COMMAND ld -o ${KERNEL_BIN} -Ttext 0x1000 ${KERNEL_ENTRY_OBJ} ${KERNEL_OBJ} --oformat binary -m elf_i386
    DEPENDS ${KERNEL_ENTRY_OBJ} ${KERNEL_OBJ}
    COMMENT "Linking Kernel"
)

# 5. Create OS Image
add_custom_target(t_os ALL
    COMMAND cat ${BOOT_BIN} ${KERNEL_BIN} > ${OS_IMAGE}
    # Pad to 1.44MB floppy size (optional, but ensures qemu treats it right)
    COMMAND truncate -s 1440k ${OS_IMAGE}
    DEPENDS ${BOOT_BIN} ${KERNEL_BIN}
    COMMENT "Creating T-OS Disk Image"
)

# 6. Run Target
add_custom_target(run
    COMMAND qemu-system-i386 -fda ${OS_IMAGE}
    DEPENDS t_os
    COMMENT "Launching T-OS in QEMU"
)
