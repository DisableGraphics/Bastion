add_subdirectory(disk)
add_subdirectory(tty)
add_subdirectory(keyboard)

target_sources(kernel_platform_dependent PRIVATE cursor.cpp handlers.cpp interrupts.cpp nmi.cpp pic.cpp pit.cpp ps2.cpp serial.cpp mouse.cpp)