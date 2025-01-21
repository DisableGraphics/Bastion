add_subdirectory(liballoc)
add_subdirectory(cpp)
add_subdirectory(time)
add_subdirectory(scheduler)

target_sources(kernel_obj PRIVATE kernel.cpp mmanager.cpp panic.cpp test.cpp key_event.cpp log.cpp)