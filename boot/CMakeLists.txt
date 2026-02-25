cmake_minimum_required(VERSION 3.10)
project(boot C)

# Includes
include_directories(/usr/include/efi)
include_directories(/usr/include/efi/x86_64)
include_directories(/usr/include/efi/protocol)

# Compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-stack-protector -fpic -fshort-wchar -mno-red-zone -I/usr/include/efi -I/usr/include/efi/x86_64 -DEFI_FUNCTION_WRAPPER")

# Object library for main.c
add_library(boot_obj OBJECT src/main.c)

# Variables
set(EFI_LDS "/usr/lib/elf_x86_64_efi.lds")
set(EFI_CRT0 "/usr/lib/crt0-efi-x86_64.o")
set(EFI_LIB_DIR "/usr/lib")

# Custom command to link using ld directly to avoid gcc's -fPIC enforcement during link
add_custom_command(OUTPUT boot.so
    COMMAND ld -nostdlib -znocombreloc -T ${EFI_LDS} -shared -Bsymbolic -L ${EFI_LIB_DIR} ${EFI_CRT0} $<TARGET_OBJECTS:boot_obj> -o boot.so -lefi -lgnuefi
    DEPENDS boot_obj
    COMMENT "Linking boot.so with ld"
)

# Custom command to objcopy to .efi
add_custom_command(OUTPUT bootx64.efi
    COMMAND objcopy -j .text -j .sdata -j .data -j .dynamic -j .dynsym -j .rel -j .rela -j .rel.* -j .rela.* -j .reloc --target efi-app-x86_64 boot.so bootx64.efi
    DEPENDS boot.so
    COMMENT "Generating bootx64.efi"
)

# Custom target to drive the build
add_custom_target(boot_target ALL DEPENDS bootx64.efi)
