cmake_minimum_required(VERSION 3.20)
project(Bastion)

set(EXEC_NAME bastion)

enable_language(ASM)

set(CMAKE_C_STANDARD 23)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_ASM_FLAGS "-g")
set(CMAKE_C_FLAGS "-g -static-libgcc -Ofast -ffreestanding -nostdlib -lgcc -msse -msse2 \
-ffast-math -flto -funroll-loops -mfpmath=sse -fno-strict-aliasing -ftree-vectorize -fno-tree-slp-vectorize \
-fgcse-sm -fgcse-las")
set(CMAKE_CXX_FLAGS "-g -static-libgcc -Ofast -ffreestanding -nostdlib -lgcc -msse \
-funroll-loops -mfpmath=sse -fno-strict-aliasing -ftree-vectorize -fno-tree-slp-vectorize \
-fgcse-sm -fgcse-las")

# Add subdirectories for kernel and libc
add_subdirectory(kernel)
add_subdirectory(libc)

# Final executable
add_executable(${EXEC_NAME})

# Add object files from kernel and libc
target_sources(${EXEC_NAME} PRIVATE
    $<TARGET_OBJECTS:kernel_obj>
	$<TARGET_OBJECTS:kernel_platform_dependent>
    $<TARGET_OBJECTS:libc_obj>
)

# Specify the linker script
target_link_options(${EXEC_NAME} PRIVATE -T ${CMAKE_SOURCE_DIR}/kernel/arch/i386/linker.ld)

add_subdirectory(user)