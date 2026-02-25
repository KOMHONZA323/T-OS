set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Don't search for programs in the build host directories
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# For libraries and headers in the target directories
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Compiler settings
set(CMAKE_C_COMPILER gcc)
set(CMAKE_CXX_COMPILER g++)
set(CMAKE_ASM_COMPILER gcc)

# Standard flags for UEFI/freestanding
set(CMAKE_C_FLAGS "-std=c11 -fshort-wchar -mno-red-zone -fno-stack-protector -fno-builtin -ffreestanding -maccumulate-outgoing-args -Wall -Wextra -Werror" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS "-fshort-wchar -mno-red-zone -fno-stack-protector -fno-builtin -ffreestanding -maccumulate-outgoing-args -Wall -Wextra -Werror" CACHE STRING "" FORCE)
set(CMAKE_ASM_FLAGS "-mno-red-zone" CACHE STRING "" FORCE)

# Linker flags to generate PE/COFF later or ELF for kernel
set(CMAKE_EXE_LINKER_FLAGS "-nostdlib -Wl,--no-undefined" CACHE STRING "" FORCE)
