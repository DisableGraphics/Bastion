add_subdirectory(liballoc)

target_sources(kernel_obj PRIVATE cpp.cpp kernel.cpp liballoc_hooks.cpp mmanager.cpp panic.cpp test.cpp key_event.cpp icxxabi.cpp)