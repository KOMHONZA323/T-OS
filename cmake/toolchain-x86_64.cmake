set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Use the host compiler (we are on x86_64 targeting x86_64 UEFI)
set(CMAKE_C_COMPILER gcc)
set(CMAKE_CXX_COMPILER g++)
set(CMAKE_ASM_COMPILER gcc)

# Don't search for programs in the host environment
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# GNU-EFI paths: Search in common locations (Debian/Ubuntu, Fedora, Arch)
find_path(EFI_INCLUDE_DIR efi.h
    PATHS /usr/include/efi /usr/include/gnu-efi /usr/local/include/efi
)

# Also find the architecture-specific include dir (usually inside the main include dir)
find_path(EFI_ARCH_INCLUDE_DIR efibind.h
    PATHS ${EFI_INCLUDE_DIR}/x86_64 ${EFI_INCLUDE_DIR}
)

find_path(EFI_PROTOCOL_INCLUDE_DIR efivar.h
    PATHS ${EFI_INCLUDE_DIR}/protocol ${EFI_INCLUDE_DIR}
)

# Find the library path (crt0-efi-x86_64.o)
# find_library looks for libNAME.so or libNAME.a. crt0 is an object file.
# We must use find_file instead.
find_file(EFI_CRT0_PATH crt0-efi-x86_64.o
    PATHS /usr/lib /usr/lib64 /usr/lib/gnuefi /usr/lib64/gnuefi /usr/lib/efi /usr/lib64/efi
)

# Get the directory of the CRT0 file to use as library search path
if(EFI_CRT0_PATH)
    get_filename_component(EFI_LIB_DIR ${EFI_CRT0_PATH} DIRECTORY)
    set(EFI_CRT0 ${EFI_CRT0_PATH})
else()
    message(WARNING "Could not find crt0-efi-x86_64.o. Make sure gnu-efi is installed.")
    set(EFI_LIB_DIR /usr/lib) # Fallback
    set(EFI_CRT0 "/usr/lib/crt0-efi-x86_64.o")
endif()


# Compiler flags for UEFI (Freestanding, no red zone, PIC)
# Note: We append -I paths here so they are always included
set(EFI_CFLAGS "-fno-stack-protector -fpic -fshort-wchar -mno-red-zone -I${EFI_INCLUDE_DIR} -I${EFI_ARCH_INCLUDE_DIR} -I${EFI_PROTOCOL_INCLUDE_DIR} -DEFI_FUNCTION_WRAPPER")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EFI_CFLAGS}" CACHE STRING "" FORCE)

# Export variables for CMakeLists.txt to use
set(EFI_LDS "${EFI_LIB_DIR}/elf_x86_64_efi.lds" CACHE STRING "Path to EFI linker script")
set(EFI_CRT0 "${EFI_CRT0}" CACHE STRING "Path to EFI crt0 object")
set(EFI_LIB_DIR "${EFI_LIB_DIR}" CACHE STRING "Path to EFI libraries")
