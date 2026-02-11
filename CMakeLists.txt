cmake_minimum_required(VERSION 3.10)
project(T-OS C ASM_NASM)

# Compiler Flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffreestanding -m32 -g -fno-pie -fno-stack-protector")

# Paths
set(BUILD_DIR "${CMAKE_BINARY_DIR}")
set(BOOT_SRC "${CMAKE_SOURCE_DIR}/src/boot/boot.asm")
set(KERNEL_ENTRY "${CMAKE_SOURCE_DIR}/src/kernel/kernel_entry.asm")


# Find all C sources
file(GLOB_RECURSE KERNEL_SOURCES
    "${CMAKE_SOURCE_DIR}/src/kernel/*.c"
    "${CMAKE_SOURCE_DIR}/src/drivers/*.c"
)


set(BOOT_BIN "${BUILD_DIR}/boot.bin")
set(KERNEL_ENTRY_OBJ "${BUILD_DIR}/kernel_entry.o")
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


# 3. Compile Kernel C Sources
file(GLOB_RECURSE KERNEL_SOURCES "${CMAKE_SOURCE_DIR}/src/kernel/*.c" "${CMAKE_SOURCE_DIR}/src/drivers/*.c")

set(KERNEL_OBJECTS "")
foreach(SOURCE_FILE ${KERNEL_SOURCES})
    get_filename_component(FILE_NAME ${SOURCE_FILE} NAME_WE)
    set(OBJECT_FILE "${BUILD_DIR}/${FILE_NAME}.o")
    list(APPEND KERNEL_OBJECTS ${OBJECT_FILE})

    add_custom_command(OUTPUT ${OBJECT_FILE}
        COMMAND gcc -ffreestanding -m32 -g -fno-pie -fno-pic -fno-stack-protector -c ${SOURCE_FILE} -o ${OBJECT_FILE}
=======
# 3. Compile Kernel Sources
set(KERNEL_OBJECTS "")
foreach(SOURCE_FILE ${KERNEL_SOURCES})
    get_filename_component(FILE_NAME ${SOURCE_FILE} NAME_WE)
    set(OBJ_FILE "${BUILD_DIR}/${FILE_NAME}.o")
    list(APPEND KERNEL_OBJECTS ${OBJ_FILE})

    add_custom_command(OUTPUT ${OBJ_FILE}
        COMMAND gcc -ffreestanding -m32 -g -fno-pie -fno-pic -fno-stack-protector -I${CMAKE_SOURCE_DIR}/src/drivers -c ${SOURCE_FILE} -o ${OBJ_FILE}

        DEPENDS ${SOURCE_FILE}
        COMMENT "Compiling ${FILE_NAME}"
    )
endforeach()

# 4. Link Kernel
add_custom_command(OUTPUT ${KERNEL_BIN}
    COMMAND ld -o ${KERNEL_BIN} -Ttext 0x1000 ${KERNEL_ENTRY_OBJ} ${KERNEL_OBJECTS} --oformat binary -m elf_i386
    DEPENDS ${KERNEL_ENTRY_OBJ} ${KERNEL_OBJECTS}
    COMMENT "Linking Kernel"
)

# 5. Create OS Image
add_custom_target(t_os ALL
    COMMAND cat ${BOOT_BIN} ${KERNEL_BIN} > ${OS_IMAGE}
    # Pad to 1.44MB floppy size
    COMMAND truncate -s 1440k ${OS_IMAGE}
    DEPENDS ${BOOT_BIN} ${KERNEL_BIN}
    COMMENT "Creating T-OS Disk Image"
)

# 6. Create Bootable ISO (El Torito floppy emulation)
set(ISO_IMAGE "${BUILD_DIR}/t-os.iso")
add_custom_target(iso ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory ${BUILD_DIR}/iso
    COMMAND ${CMAKE_COMMAND} -E copy ${OS_IMAGE} ${BUILD_DIR}/iso/
    COMMAND genisoimage -quiet -V "T-OS" -input-charset iso8859-1 -o ${ISO_IMAGE}
        -b t-os.bin -c boot.cat
        ${BUILD_DIR}/iso
    DEPENDS t_os
    COMMENT "Creating T-OS ISO image"
    WORKING_DIRECTORY ${BUILD_DIR}
)

# 7. Run Target
add_custom_target(run
    COMMAND qemu-system-i386 -fda ${OS_IMAGE}
    DEPENDS t_os
    COMMENT "Launching T-OS in QEMU"
)

# 8. Run from ISO
add_custom_target(run_iso
    COMMAND qemu-system-i386 -cdrom ${ISO_IMAGE}
    DEPENDS iso
    COMMENT "Launching T-OS from ISO in QEMU"
)
