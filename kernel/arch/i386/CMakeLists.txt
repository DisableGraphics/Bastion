add_library(kernel_platform_dependent)

target_compile_options(kernel_platform_dependent PRIVATE -fno-asynchronous-unwind-tables -fno-exceptions -ffreestanding -nostdlib -mgeneral-regs-only -lgcc)

target_include_directories(kernel_platform_dependent PUBLIC ${CMAKE_SOURCE_DIR}/kernel/include/)
target_include_directories(kernel_platform_dependent PUBLIC ${CMAKE_SOURCE_DIR}/libc/include/)

target_sources(kernel_platform_dependent PRIVATE
	gdt.cpp
	handlers.cpp
	inlineasm.cpp
	interrupts.cpp
	keyboard.cpp
	mutex.cpp
	nmi.cpp
	page.cpp
	pic.cpp
	pit.cpp
	ps2.cpp
	serial.cpp
	ttycpp.cpp
	boot.S
	crti.S
	crtn.S
	enablePaging.S
	isr.S
	loadPageDirectory.S
	reloadsr.S
	keyboard_get_key.cpp
	)