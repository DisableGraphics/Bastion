cmake_minimum_required(VERSION 3.20)
project(BastionUser)

set(CMAKE_ASM_FLAGS "-g -ffreestanding -nostdlib -mgeneral-regs-only -lgcc")
set(CMAKE_C_FLAGS "-g -ffreestanding -nostdlib -mgeneral-regs-only -lgcc")
set(CMAKE_CXX_FLAGS "-g -O2 -ffreestanding -nostdlib -lgcc")

add_subdirectory(asm)
add_subdirectory(c)