set(FONT_SRC "console.sfn")  # Relative path for clean section names
set(FONT_OBJ "${CMAKE_CURRENT_BINARY_DIR}/font_sfn.o")

# Get absolute path for dependency tracking
get_filename_component(ABS_FONT_SRC "${FONT_SRC}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

if(NOT EXISTS "${ABS_FONT_SRC}")
    message(FATAL_ERROR "Font file not found: ${ABS_FONT_SRC}")
endif()

# Create object file from binary
add_custom_command(
    OUTPUT ${FONT_OBJ}
    # Change to source dir to get clean symbol names
    COMMAND ${CMAKE_COMMAND} -E chdir ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_LINKER} -r -b binary -o ${FONT_OBJ} ${FONT_SRC}
    DEPENDS ${ABS_FONT_SRC}
    COMMENT "Converting font to object: ${FONT_SRC} -> ${FONT_OBJ}"
    VERBATIM
)

# Custom target to ensure the object gets built
add_custom_target(font_object DEPENDS ${FONT_OBJ})

# Make sure kernel_obj knows about this object file
if(TARGET kernel_platform_dependent)
    # For OBJECT libraries:
    target_sources(kernel_platform_dependent PRIVATE ${FONT_OBJ})
    
    # For regular libraries/executables, you might need:
    target_link_libraries(kernel_platform_dependent PRIVATE ${FONT_OBJ})
    
    add_dependencies(kernel_platform_dependent font_object)
else()
    message(WARNING "Target kernel_platform_dependent not found - font object won't be linked")
endif()
