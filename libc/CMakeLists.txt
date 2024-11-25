# Build libc object files
add_library(libc_obj)

add_subdirectory(stdio)
add_subdirectory(string)
add_subdirectory(stdlib)

# Include libc headers
target_include_directories(libc_obj PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Include kernel headers for compilation
target_include_directories(libc_obj PRIVATE ${CMAKE_SOURCE_DIR}/kernel/include)
