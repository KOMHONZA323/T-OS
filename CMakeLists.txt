cmake_minimum_required(VERSION 3.10)
project(T-OS C ASM)

# Add the bootloader directory
add_subdirectory(boot)

# Add a custom target to build the disk image
add_custom_target(image
    COMMAND ${CMAKE_SOURCE_DIR}/scripts/build_image.sh
    DEPENDS boot
    COMMENT "Building T-OS Disk Image (tos.img)..."
)
