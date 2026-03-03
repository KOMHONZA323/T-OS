cmake_minimum_required(VERSION 3.10)

<<<<<<< HEAD
# Add the toolchain file path for convenience if not provided
# This must be done BEFORE the project() command, because project() enables languages
# and triggers the toolchain loading.
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/cmake/toolchain-x86_64.cmake)
endif()

project(T-OS C ASM)

# Add the bootloader directory
add_subdirectory(boot)
=======
# Toolchain paths
set(EFI_INC "-I/usr/include/efi -I/usr/include/efi/x86_64")
set(EFI_LDS "/usr/lib/elf_x86_64_efi.lds")
set(EFI_CRT "/usr/lib/crt0-efi-x86_64.o")
set(EFI_LIB "/usr/lib")

# 1. Bootloader
add_library(boot_obj OBJECT main.c)
set_target_properties(boot_obj PROPERTIES COMPILE_FLAGS "-fno-stack-protector -fpic -fshort-wchar -mno-red-zone -ffreestanding ${EFI_INC}")
>>>>>>> 9c1317d (New)

add_custom_command(OUTPUT bootx64.efi
    COMMAND gcc -nostdlib -fPIC -shared -Wl,-Bsymbolic -Wl,-T,${EFI_LDS} -L${EFI_LIB} ${EFI_CRT} $<TARGET_OBJECTS:boot_obj> -o boot.so -lefi -lgnuefi
    COMMAND objcopy -j .text -j .sdata -j .data -j .dynamic -j .dynsym -j .rel -j .rela -j .rel.* -j .rela.* -j .reloc --target efi-app-x86_64 boot.so bootx64.efi
    DEPENDS boot_obj
)
add_custom_target(boot ALL DEPENDS bootx64.efi)

# 2. Kernel
add_library(kernel_obj OBJECT kernel.c entry.s)
set_target_properties(kernel_obj PROPERTIES COMPILE_FLAGS "-ffreestanding -mno-red-zone -m64")

add_custom_command(OUTPUT kernel.bin
    COMMAND ld -nostdlib -Ttext 0x1000000 -e _start $<TARGET_OBJECTS:kernel_obj> -o kernel.elf
    COMMAND objcopy -O binary kernel.elf kernel.bin
    DEPENDS kernel_obj
    COMMAND_EXPAND_LISTS
)
add_custom_target(kernel ALL DEPENDS kernel.bin)

# 3. Disk Image (GPT + ESP)
add_custom_target(image ALL
    COMMAND dd if=/dev/zero of=tos.img bs=1M count=66
    COMMAND parted -s tos.img mklabel gpt
    COMMAND parted -s tos.img mkpart ESP fat32 2048s 100%
    COMMAND parted -s tos.img set 1 esp on
    COMMAND dd if=/dev/zero of=part.img bs=1M count=64
    COMMAND mkfs.vfat -F 32 part.img
    COMMAND mmd -i part.img ::/EFI
    COMMAND mmd -i part.img ::/EFI/BOOT
    COMMAND mcopy -i part.img bootx64.efi ::/EFI/BOOT/BOOTX64.EFI
    COMMAND mcopy -i part.img kernel.bin ::/kernel.bin
    COMMAND dd if=part.img of=tos.img bs=1M seek=1 conv=notrunc
    COMMAND rm part.img
    DEPENDS boot kernel
)
