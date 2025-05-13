add_subdirectory(inodes)

target_sources(kernel_obj PRIVATE vfs.cpp)