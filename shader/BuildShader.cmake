
set(SHADER_FILES
    model.frag
    model.vert
    particle.comp
    particle.frag
    particle.vert
    point_light.frag
    point_light.vert
)

# 查找 glslc 编译器
find_program(GLSLC_PROGRAM glslc REQUIRED)

# 设置输出目录（相对于项目根目录）
set(SHADER_OUTPUT_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/data/shader)
file(MAKE_DIRECTORY ${SHADER_OUTPUT_DIR})

foreach(SHADER_FILE ${SHADER_FILES})
    get_filename_component(SHADER_NAME ${SHADER_FILE} NAME_WE)
    get_filename_component(SHADER_EXT ${SHADER_FILE} EXT)
    string(REPLACE "." "_" EXT_CLEAN ${SHADER_EXT})
    set(OUTPUT_NAME "${SHADER_NAME}${EXT_CLEAN}.spv")

    set(SHADER_SOURCE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/${SHADER_FILE})
    set(SHADER_OUTPUT_PATH ${SHADER_OUTPUT_DIR}/${OUTPUT_NAME})

    execute_process(COMMAND ${GLSLC_PROGRAM} ${SHADER_SOURCE_PATH} -o ${SHADER_OUTPUT_PATH})
    message(STATUS "Compiled shader: ${SHADER_FILE} -> ${OUTPUT_NAME}")
endforeach()
