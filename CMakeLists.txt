cmake_minimum_required(VERSION 3.10)

# Add the toolchain file path for convenience if not provided
# This must be done BEFORE the project() command, because project() enables languages
# and triggers the toolchain loading.
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/cmake/toolchain-x86_64.cmake)
endif()

project(T-OS C ASM)

# Add the bootloader directory
add_subdirectory(boot)

# Add a custom target to build the disk image
add_custom_target(image
    COMMAND ${CMAKE_SOURCE_DIR}/scripts/build_image.sh
    DEPENDS boot
    COMMENT "Building T-OS Disk Image (tos.img)..."
)
