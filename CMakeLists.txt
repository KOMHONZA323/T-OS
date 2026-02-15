cmake_minimum_required(VERSION 3.10)
project(T-OS C ASM_NASM)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffreestanding -m32 -g -fno-pie -fno-stack-protector")

set(BUILD_DIR "${CMAKE_BINARY_DIR}")
set(BOOT_SRC "${CMAKE_SOURCE_DIR}/src/boot/stage1/boot_sect.asm")
set(STAGE2_SRC "${CMAKE_SOURCE_DIR}/src/boot/stage2/stage2.asm")
set(KERNEL_ENTRY "${CMAKE_SOURCE_DIR}/src/kernel/kernel_entry.asm")
set(INTERRUPT_ASM "${CMAKE_SOURCE_DIR}/src/kernel/interrupt.asm")

set(BOOT_BIN "${BUILD_DIR}/boot.bin")
set(STAGE2_BIN "${BUILD_DIR}/stage2.bin")
set(KERNEL_ENTRY_OBJ "${BUILD_DIR}/kernel_entry.o")
set(INTERRUPT_OBJ "${BUILD_DIR}/interrupt.o")
set(KERNEL_BIN "${BUILD_DIR}/kernel.bin")
set(OS_IMAGE "${BUILD_DIR}/t-os.bin")

add_custom_command(OUTPUT ${BOOT_BIN}
    COMMAND nasm -f bin ${BOOT_SRC} -o ${BOOT_BIN} -I ${CMAKE_SOURCE_DIR}/
    DEPENDS ${BOOT_SRC} "${CMAKE_SOURCE_DIR}/src/boot/stage1/disk_load.asm"
    COMMENT "Assembling Bootloader Stage 1"
)

add_custom_command(OUTPUT ${STAGE2_BIN}
    COMMAND nasm -f bin ${STAGE2_SRC} -o ${STAGE2_BIN} -I ${CMAKE_SOURCE_DIR}/
    DEPENDS ${STAGE2_SRC}
            "${CMAKE_SOURCE_DIR}/src/boot/stage2/print_string_pm.asm"
            "${CMAKE_SOURCE_DIR}/src/boot/stage2/switch_to_pm.asm"
            "${CMAKE_SOURCE_DIR}/src/boot/stage2/gdt.asm"
            "${CMAKE_SOURCE_DIR}/src/boot/stage2/utils.asm"
            "${CMAKE_SOURCE_DIR}/src/boot/stage2/disk.asm"
            "${CMAKE_SOURCE_DIR}/src/boot/stage2/vbe.asm"
            "${CMAKE_SOURCE_DIR}/src/boot/stage2/menu.asm"
            "${CMAKE_SOURCE_DIR}/src/boot/stage2/data.asm"
            "${CMAKE_SOURCE_DIR}/src/boot/stage2/memory_map.asm"
    COMMENT "Assembling Bootloader Stage 2"
)

add_custom_command(OUTPUT ${KERNEL_ENTRY_OBJ}
    COMMAND nasm -f elf ${KERNEL_ENTRY} -o ${KERNEL_ENTRY_OBJ}
    DEPENDS ${KERNEL_ENTRY}
    COMMENT "Assembling Kernel Entry"
)

add_custom_command(OUTPUT ${INTERRUPT_OBJ}
    COMMAND nasm -f elf ${INTERRUPT_ASM} -o ${INTERRUPT_OBJ}
    DEPENDS ${INTERRUPT_ASM}
    COMMENT "Assembling Interrupt Handlers"
)

file(GLOB_RECURSE KERNEL_SOURCES "${CMAKE_SOURCE_DIR}/src/kernel/*.c" "${CMAKE_SOURCE_DIR}/src/drivers/*.c")
file(GLOB SYSCALL_SOURCES "${CMAKE_SOURCE_DIR}/src/kernel/syscalls/*.c")
list(APPEND KERNEL_SOURCES ${SYSCALL_SOURCES})
list(REMOVE_DUPLICATES KERNEL_SOURCES)

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

add_custom_command(OUTPUT ${KERNEL_BIN}
    COMMAND ld -o ${KERNEL_BIN} -Ttext 0x10000 ${KERNEL_ENTRY_OBJ} ${INTERRUPT_OBJ} ${KERNEL_OBJECTS} --oformat binary -m elf_i386
    DEPENDS ${KERNEL_ENTRY_OBJ} ${INTERRUPT_OBJ} ${KERNEL_OBJECTS}
    COMMENT "Linking Kernel"
)

set(SDK_DIR "${CMAKE_SOURCE_DIR}/src/sdk")
set(TASM_SRC "${SDK_DIR}/tasm.c")
set(STD_SRC "${SDK_DIR}/std.c")
set(CRT0_ASM "${SDK_DIR}/crt0.asm")
set(SYSCALLS_ASM "${SDK_DIR}/syscalls.asm")
set(USER_LD "${SDK_DIR}/user.ld")

set(TASM_OBJ "${BUILD_DIR}/tasm.o")
set(STD_OBJ "${BUILD_DIR}/std.o")
set(CRT0_OBJ "${BUILD_DIR}/crt0.o")
set(SYSCALLS_OBJ "${BUILD_DIR}/syscalls.o")
set(TASM_BIN "${BUILD_DIR}/tasm.bin")
set(TASM_TEXF "${BUILD_DIR}/tasm.texf")

add_custom_command(OUTPUT ${TASM_OBJ}
    COMMAND gcc -ffreestanding -m32 -g -fno-pie -fno-pic -I${SDK_DIR} -c ${TASM_SRC} -o ${TASM_OBJ}
    DEPENDS ${TASM_SRC}
)
add_custom_command(OUTPUT ${STD_OBJ}
    COMMAND gcc -ffreestanding -m32 -g -fno-pie -fno-pic -I${SDK_DIR} -c ${STD_SRC} -o ${STD_OBJ}
    DEPENDS ${STD_SRC}
)
add_custom_command(OUTPUT ${CRT0_OBJ}
    COMMAND nasm -f elf ${CRT0_ASM} -o ${CRT0_OBJ}
    DEPENDS ${CRT0_ASM}
)
add_custom_command(OUTPUT ${SYSCALLS_OBJ}
    COMMAND nasm -f elf ${SYSCALLS_ASM} -o ${SYSCALLS_OBJ}
    DEPENDS ${SYSCALLS_ASM}
)

add_custom_command(OUTPUT ${TASM_BIN}
    COMMAND ld -T ${USER_LD} -o ${TASM_BIN} ${CRT0_OBJ} ${SYSCALLS_OBJ} ${TASM_OBJ} ${STD_OBJ} --oformat binary -m elf_i386
    DEPENDS ${CRT0_OBJ} ${SYSCALLS_OBJ} ${TASM_OBJ} ${STD_OBJ} ${USER_LD}
)

add_custom_command(OUTPUT ${TASM_TEXF}
    COMMAND python3 ${SDK_DIR}/make_texf.py ${TASM_BIN} ${TASM_TEXF}
    DEPENDS ${TASM_BIN}
)

add_custom_target(t_os ALL
    COMMAND cat ${STAGE2_BIN} ${KERNEL_BIN} > ${BUILD_DIR}/stage2_kernel.bin
    COMMAND python3 ${SDK_DIR}/make_fat.py ${BOOT_BIN} ${BUILD_DIR}/stage2_kernel.bin ${OS_IMAGE} tasm.texf=${TASM_TEXF} test.asm=${SDK_DIR}/test.asm
    COMMAND ls -l ${OS_IMAGE}
    DEPENDS ${BOOT_BIN} ${STAGE2_BIN} ${KERNEL_BIN} ${TASM_TEXF}
    COMMENT "Creating T-OS Disk Image (FAT)"
)

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

add_custom_target(run
    COMMAND qemu-system-i386 -drive file=${OS_IMAGE},format=raw,index=0,if=floppy -vga std -usb -device usb-tablet -serial stdio
    DEPENDS t_os
    COMMENT "Launching T-OS in QEMU"
)

add_custom_target(run_iso
    COMMAND qemu-system-i386 -cdrom ${ISO_IMAGE} -vga std
    DEPENDS iso
    COMMENT "Launching T-OS from ISO in QEMU"
)
