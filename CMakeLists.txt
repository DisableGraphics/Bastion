cmake_minimum_required(VERSION 3.20)
project(Bastion)

set(EXEC_NAME bastion)

enable_language(ASM)

set(CMAKE_C_STANDARD 23)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_ASM_FLAGS "-g -fno-pie -no-pie")
set(CMAKE_C_FLAGS "-g -static-libgcc -O2 -ffreestanding -nostdlib -lgcc -msse -msse2 -Wall -Wextra -Wpedantic -fno-pie -no-pie -DDEBUG -fsanitize=undefined -fno-sanitize=alignment")
set(CMAKE_CXX_FLAGS "-g -static-libgcc -O2 -ffreestanding -nostdlib -lgcc -fno-pie -no-pie -DDEBUG -fsanitize=undefined")


set(SYMBOL_TABLE_C ${CMAKE_BINARY_DIR}/symbol_table.cpp)
set(TEMP_ELF ${CMAKE_BINARY_DIR}/${EXEC_NAME}.tmp)

# Add subdirectories for kernel and libc
add_subdirectory(kernel)
add_subdirectory(libc)

# Final executable
add_executable(${EXEC_NAME}.tmp)

# Add object files from kernel and libc
target_sources(${EXEC_NAME}.tmp PRIVATE
	$<TARGET_OBJECTS:kernel_obj_tmp>
    $<TARGET_OBJECTS:kernel_obj>
	$<TARGET_OBJECTS:kernel_platform_dependent>
    $<TARGET_OBJECTS:libc_obj>
)

# Specify the linker script
target_link_options(${EXEC_NAME}.tmp PRIVATE -T ${CMAKE_SOURCE_DIR}/kernel/arch/i386/linker.ld)

add_custom_command(
    OUTPUT ${SYMBOL_TABLE_C}
    COMMAND ${CMAKE_SOURCE_DIR}/scripts/generate_symbol_table.py ${TEMP_ELF} ${SYMBOL_TABLE_C}
    DEPENDS ${EXEC_NAME}.tmp
    COMMENT "Generating symbol table from ${TEMP_ELF}"
)

add_custom_target(generate_symbol_table DEPENDS ${SYMBOL_TABLE_C})
add_library(kernel_symbols_obj OBJECT ${SYMBOL_TABLE_C})
add_dependencies(kernel_symbols_obj generate_symbol_table)

add_executable(${EXEC_NAME})

target_sources(${EXEC_NAME} PRIVATE
    $<TARGET_OBJECTS:kernel_obj>
    $<TARGET_OBJECTS:kernel_platform_dependent>
    $<TARGET_OBJECTS:libc_obj>
    $<TARGET_OBJECTS:kernel_symbols_obj>
)

target_link_options(${EXEC_NAME} PRIVATE -T ${CMAKE_SOURCE_DIR}/kernel/arch/i386/linker.ld)

add_subdirectory(user)