cmake_minimum_required(VERSION 3.10)
project(T-OS C ASM)

# Add the toolchain file path for convenience if not provided
if(NOT CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/cmake/toolchain-x86_64.cmake)
endif()

# Add the bootloader directory
add_subdirectory(boot)

# Add a custom target to build the disk image
add_custom_target(image
    COMMAND ${CMAKE_SOURCE_DIR}/scripts/build_image.sh
    DEPENDS boot
    COMMENT "Building T-OS Disk Image (tos.img)..."
)
