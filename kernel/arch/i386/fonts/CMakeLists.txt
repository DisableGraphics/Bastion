set(FONT_SRC "console.sfn")  # Relative path for clean section names
set(FONT_OBJ "${CMAKE_CURRENT_BINARY_DIR}/font_sfn.o")

# Get absolute path for dependency tracking
get_filename_component(ABS_FONT_SRC "${FONT_SRC}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

if(NOT EXISTS "${ABS_FONT_SRC}")
    message(FATAL_ERROR "Font file not found: ${ABS_FONT_SRC}")
endif()

add_library(font_obj STATIC EXCLUDE_FROM_ALL ${FONT_OBJ})
SET_SOURCE_FILES_PROPERTIES(
  ${FONT_OBJ}
  PROPERTIES
  EXTERNAL_OBJECT true
  GENERATED true
)
SET_TARGET_PROPERTIES(
  font_obj
  PROPERTIES
  LINKER_LANGUAGE C
  ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib
)

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

target_link_libraries(kernel_platform_dependent PRIVATE font_obj)