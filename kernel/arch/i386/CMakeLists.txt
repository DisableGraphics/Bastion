add_library(kernel_platform_dependent)

add_subdirectory(bits)
add_subdirectory(drivers)
add_subdirectory(memory)
add_subdirectory(boot)
add_subdirectory(assembly)
add_subdirectory(sync)
add_subdirectory(fs)
add_subdirectory(random)
add_subdirectory(scheduler)

target_compile_options(kernel_platform_dependent PRIVATE -g -fno-asynchronous-unwind-tables -fno-exceptions -fno-rtti -fno-sized-deallocation -ffreestanding -nostdlib -mgeneral-regs-only -lgcc)

target_include_directories(kernel_platform_dependent PUBLIC ${CMAKE_SOURCE_DIR}/kernel/include/)
target_include_directories(kernel_platform_dependent PUBLIC ${CMAKE_SOURCE_DIR}/libc/include/)