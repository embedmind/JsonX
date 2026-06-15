# ARM GCC toolchain for Windows.
#
# Usage:
#   set ARM_GCC_ROOT=D:\path\to\arm-gnu-toolchain
#   cmake -S . -B build-arm -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-arm-gcc-win.cmake
#   cmake --build build-arm

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

if(NOT DEFINED ARM_GCC_ROOT OR ARM_GCC_ROOT STREQUAL "")
    set(ARM_GCC_ROOT "$ENV{ARM_GCC_ROOT}" CACHE PATH "ARM GCC toolchain root")
endif()

if(NOT ARM_GCC_ROOT)
    message(FATAL_ERROR
        "ARM_GCC_ROOT is not set. Set the ARM_GCC_ROOT environment variable "
        "or pass -DARM_GCC_ROOT=<path-to-arm-gcc-toolchain-root>.")
endif()

set(CMAKE_C_COMPILER   "${ARM_GCC_ROOT}/bin/arm-none-eabi-gcc.exe")
set(CMAKE_ASM_COMPILER "${ARM_GCC_ROOT}/bin/arm-none-eabi-gcc.exe")
set(CMAKE_OBJCOPY      "${ARM_GCC_ROOT}/bin/arm-none-eabi-objcopy.exe")
set(CMAKE_SIZE         "${ARM_GCC_ROOT}/bin/arm-none-eabi-size.exe")

# Allows standalone compile/link smoke checks without project-provided syscalls.
set(CMAKE_EXE_LINKER_FLAGS_INIT "--specs=nosys.specs")

# Cross-compilation checks must not try to run target binaries.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
