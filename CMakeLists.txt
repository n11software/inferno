cmake_minimum_required(VERSION 3.16)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -z noexecstack")

set(CMAKE_C_COMPILER_WORKS 1)
set(CMAKE_CXX_COMPILER_WORKS 1)

SET(CMAKE_SYSTEM_NAME Generic)

SET(CMAKE_SYSTEM_PROCESSOR x86_64)
SET(CMAKE_CROSSCOMPILING 1)

set(CMAKE_C_COMPILER "x86_64-hydra-gcc")
set(CMAKE_CXX_COMPILER "x86_64-hydra-g++")

project(Inferno C CXX ASM_NASM)

add_definitions(-DkernelVersion="0.1alpha")

# Setup NASM
set(CMAKE_ASM_NASM_SOURCE_FILE_EXTENSIONS ${CMAKE_ASM_NASM_SOURCE_FILE_EXTENSIONS} s S)
enable_language(ASM_NASM)
set(CMAKE_ASM_NASM_OBJECT_FORMAT elf64)
set(CMAKE_ASM_NASM_COMPILE_OBJECT "nasm <SOURCE> -f elf64 -o <OBJECT>")

# Find source files
file(GLOB_RECURSE CPP_SRCS Source/*.cpp)
file(GLOB_RECURSE ASM_SRCS Source/*.s)
set_source_files_properties(${ASM_SRCS} PROPERTIES LANGUAGE ASM_NASM)

# Compile
set(CMAKE_CXX_FLAGS "-ffreestanding -fshort-wchar -mabi=sysv -no-pie -mno-red-zone -fpermissive -mgeneral-regs-only -O2 -fno-stack-protector -fno-exceptions -nostdlib -Wl,--no-warn-rwx-segments -Wno-int-to-pointer-cast -Wno-permissive -mcmodel=kernel")
add_link_options(-T${CMAKE_CURRENT_SOURCE_DIR}/linker.ld -static -Bsymbolic -nostdlib -e main -Wl,--no-warn-rwx-segments)
include_directories(Include)
add_executable(kernel ${CPP_SRCS} ${ASM_SRCS})

set_target_properties(kernel PROPERTIES LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/linker.ld)
