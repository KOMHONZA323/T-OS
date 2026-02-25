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

# GNU-EFI paths (Ubuntu/Debian layout)
set(EFI_INCLUDE_DIR /usr/include/efi)
set(EFI_LIB_DIR /usr/lib)

# Compiler flags for UEFI (Freestanding, no red zone, PIC)
set(EFI_CFLAGS "-fno-stack-protector -fpic -fshort-wchar -mno-red-zone -I${EFI_INCLUDE_DIR} -I${EFI_INCLUDE_DIR}/x86_64 -DEFI_FUNCTION_WRAPPER")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EFI_CFLAGS}" CACHE STRING "" FORCE)
