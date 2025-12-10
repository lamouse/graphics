set(FONT_SOURCE_DIR ${CMAKE_SOURCE_DIR}/fonts)
set(FONT_OUTPUT_DIR ${CMAKE_BINARY_DIR}/bin/fonts)
file(MAKE_DIRECTORY "${FONT_OUTPUT_DIR}")
set(FRONT_EXTENSIONS ttf otf)
set(FONT_FILES "")

foreach(ext IN LISTS FRONT_EXTENSIONS)
    file(GLOB_RECURSE tmp
        LIST_DIRECTORIES false
        RELATIVE "${FONT_SOURCE_DIR}"
        CONFIGURE_DEPENDS
        "${FONT_SOURCE_DIR}/*.${ext}"
    )
    list(APPEND FONT_FILES ${tmp})
endforeach()


set(FONT_OUT_PATHS "")
foreach(FONT_FILE IN LISTS FONT_FILES)
    set(FONT_OUT_PATH ${FONT_OUTPUT_DIR}/${FONT_FILE})
    add_custom_command(
        OUTPUT ${FONT_OUT_PATH}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${FONT_SOURCE_DIR}/${FONT_FILE}" "${FONT_OUT_PATH}"
    )
    list(APPEND FONT_OUT_PATHS ${FONT_OUT_PATH})
endforeach()

add_custom_target(copy_fonts ALL
    DEPENDS ${FONT_OUT_PATHS}
)