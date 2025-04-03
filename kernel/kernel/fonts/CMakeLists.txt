set(FONT_SRC "${CMAKE_CURRENT_SOURCE_DIR}/RougeScript-Regular.sfn")
set(FONT_OBJ "${CMAKE_CURRENT_BINARY_DIR}/font_sfn.o")

if(NOT EXISTS "${FONT_SRC}")
    message(FATAL_ERROR "Font file not found: ${FONT_SRC}")
endif()

add_custom_command(
    OUTPUT ${FONT_OBJ}
    COMMAND ${CMAKE_LINKER} -r -b binary -o ${FONT_OBJ} ${FONT_SRC}
    DEPENDS ${FONT_SRC}
    COMMENT "Converting font to object: ${FONT_SRC} -> ${FONT_OBJ}"
    VERBATIM
)

add_custom_target(font_object ALL DEPENDS ${FONT_OBJ})

target_sources(kernel_obj PRIVATE ${FONT_OBJ})

add_dependencies(kernel_obj font_object)