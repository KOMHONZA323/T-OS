cmake_minimum_required(VERSION 3.16)
project(bootloader LANGUAGES C)

# Define the source files
set(SOURCES src/main.c)

# Add the executable target
add_executable(bootloader ${SOURCES})

# Specify linker flags for creating a UEFI executable
# We are building a freestanding PE32+ executable without GNU-EFI libraries.

include_directories(
    ${CMAKE_SOURCE_DIR}/boot/include
)

# For x86_64 UEFI
target_compile_definitions(bootloader PRIVATE
    CONFIG_X86_64
    __x86_64__
)

target_compile_options(bootloader PRIVATE
    -fpic
    -fshort-wchar
    -mno-red-zone
    -maccumulate-outgoing-args
    -fno-stack-protector
    -fno-builtin
    -Wall
    -Werror
    -ffreestanding
)

# We link with -shared to create a position-independent executable suitable for UEFI conversion
# Also need to specify entry point
target_link_options(bootloader PRIVATE
    -shared
    -Bsymbolic
    -L${CMAKE_SOURCE_DIR}/boot/lib
    -T${CMAKE_SOURCE_DIR}/boot/linker.ld
    -e efi_main
    -z noexecstack
)

# The default output name will be `bootloader`. We will rename/convert it later.
set_target_properties(bootloader PROPERTIES OUTPUT_NAME "bootx64.efi")

# Convert ELF to EFI PE32+ format using objcopy
add_custom_command(TARGET bootloader POST_BUILD
    COMMAND objcopy
        -j .text -j .sdata -j .data -j .dynamic -j .dynsym -j .rel -j .rela -j .rel.* -j .rela.* -j .rel* -j .rela* -j .reloc
        --target=efi-app-x86_64
        $<TARGET_FILE:bootloader>
        ${CMAKE_BINARY_DIR}/boot/bootx64.efi
    COMMENT "Converting ELF to EFI PE32+ format"
)
