cmake_minimum_required(VERSION 3.16)
project(T-OS VERSION 0.1 LANGUAGES C ASM)

# Add the CMAKE_TOOLCHAIN_FILE
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/cmake/toolchain-x86_64.cmake CACHE PATH "The path to the toolchain file")

# Set the output directory for all executables and libraries
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Add subdirectories
add_subdirectory(boot)

# Custom target to build the disk image
add_custom_target(tos_image
    COMMAND ${CMAKE_SOURCE_DIR}/scripts/build_image.sh ${CMAKE_BINARY_DIR}
    DEPENDS bootloader
    COMMENT "Building T-OS disk image (tos.img)..."
)
