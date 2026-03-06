cmake_minimum_required(VERSION 3.10)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/cmake/toolchain-x86_64.cmake)
endif()

project(T-OS C ASM)

# 1. Bootloader
set(UEFI_CC x86_64-w64-mingw32-gcc)
set(UEFI_CFLAGS -ffreestanding -fno-stack-protector -fno-stack-check -fshort-wchar -mno-red-zone -maccumulate-outgoing-args -Wall)
set(UEFI_LDFLAGS -nostdlib -Wl,-dll -shared -Wl,--subsystem,10 -e efi_main)

add_custom_command(OUTPUT bootx64.efi
    COMMAND ${UEFI_CC} ${UEFI_CFLAGS} ${UEFI_LDFLAGS} ${CMAKE_CURRENT_SOURCE_DIR}/main.c ${CMAKE_CURRENT_SOURCE_DIR}/gui/gui.c -o ${CMAKE_BINARY_DIR}/bootx64.efi
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/main.c ${CMAKE_CURRENT_SOURCE_DIR}/gui/gui.c ${CMAKE_CURRENT_SOURCE_DIR}/uefi.h ${CMAKE_CURRENT_SOURCE_DIR}/bootinfo.h
)
add_custom_target(boot ALL DEPENDS bootx64.efi)

# 2. Kernel
add_library(kernel_obj OBJECT kernel.c idt.c entry.s compositor.c)
set_target_properties(kernel_obj PROPERTIES COMPILE_FLAGS "-ffreestanding -mno-red-zone -m64 -I${CMAKE_CURRENT_SOURCE_DIR}")

add_custom_command(OUTPUT kernel.bin
    COMMAND ld -nostdlib -Ttext 0x1000000 -e _start $<TARGET_OBJECTS:kernel_obj> -o ${CMAKE_BINARY_DIR}/kernel.elf
    COMMAND objcopy -O binary ${CMAKE_BINARY_DIR}/kernel.elf ${CMAKE_BINARY_DIR}/kernel.bin
    DEPENDS kernel_obj compositor.h idt.h bootinfo.h
    COMMAND_EXPAND_LISTS
)
add_custom_target(kernel ALL DEPENDS kernel.bin)

# 3. Disk Image (GPT + ESP)
add_custom_target(image ALL
    COMMAND dd if=/dev/zero of=${CMAKE_BINARY_DIR}/tos.img bs=1M count=66
    COMMAND parted -s ${CMAKE_BINARY_DIR}/tos.img mklabel gpt
    COMMAND parted -s ${CMAKE_BINARY_DIR}/tos.img mkpart ESP fat32 2048s 100%
    COMMAND parted -s ${CMAKE_BINARY_DIR}/tos.img set 1 esp on
    COMMAND dd if=/dev/zero of=${CMAKE_BINARY_DIR}/part.img bs=1M count=64
    COMMAND mkfs.vfat -F 32 ${CMAKE_BINARY_DIR}/part.img
    COMMAND mmd -i ${CMAKE_BINARY_DIR}/part.img ::/EFI
    COMMAND mmd -i ${CMAKE_BINARY_DIR}/part.img ::/EFI/BOOT
    COMMAND mmd -i ${CMAKE_BINARY_DIR}/part.img ::/include
    COMMAND mmd -i ${CMAKE_BINARY_DIR}/part.img ::/src
    COMMAND mcopy -i ${CMAKE_BINARY_DIR}/part.img ${CMAKE_BINARY_DIR}/bootx64.efi ::/EFI/BOOT/BOOTX64.EFI
    COMMAND mcopy -i ${CMAKE_BINARY_DIR}/part.img ${CMAKE_BINARY_DIR}/kernel.bin ::/kernel.bin
    COMMAND mcopy -i ${CMAKE_BINARY_DIR}/part.img ${CMAKE_CURRENT_SOURCE_DIR}/hello.c ::/hello.c
    COMMAND mcopy -i ${CMAKE_BINARY_DIR}/part.img ${CMAKE_CURRENT_SOURCE_DIR}/test.hc ::/test.hc
    COMMAND mcopy -i ${CMAKE_BINARY_DIR}/part.img ${CMAKE_CURRENT_SOURCE_DIR}/game.c ::/game.c
    COMMAND mcopy -i ${CMAKE_BINARY_DIR}/part.img ${CMAKE_CURRENT_SOURCE_DIR}/HOWTO.txt ::/HOWTO.txt
    COMMAND dd if=${CMAKE_BINARY_DIR}/part.img of=${CMAKE_BINARY_DIR}/tos.img bs=1M seek=1 conv=notrunc
    COMMAND rm -f ${CMAKE_BINARY_DIR}/part.img
    DEPENDS boot kernel
)
