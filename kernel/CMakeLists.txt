# Build kernel object files
add_library(kernel_obj)

target_compile_options(kernel_obj PRIVATE -O2 -D__is_kernel -fno-asynchronous-unwind-tables -fno-exceptions -ffreestanding -fno-rtti -nostdlib -lgcc -fno-sized-deallocation)

# Include kernel headers
target_include_directories(kernel_obj PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_include_directories(kernel_obj PUBLIC ${CMAKE_SOURCE_DIR}/libc/include)

add_subdirectory(kernel)
add_subdirectory(arch/i386)