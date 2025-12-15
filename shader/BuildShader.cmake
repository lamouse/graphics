
set(SHADER_FILES
    animation.frag
    animation.vert
    model.frag
    model.vert
    particle.comp
    particle.frag
    particle.vert
    point_light.frag
    point_light.vert
    skycube.frag
    skycube.vert
)

# 查找 glslc 编译器
find_program(GLSLC_PROGRAM glslc REQUIRED)

# 设置输出目录（相对于项目根目录）
set(SHADER_OUTPUT_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/data/shader)
file(MAKE_DIRECTORY ${SHADER_OUTPUT_DIR})

# 收集所有输出文件路径
set(SHADER_OUTPUTS "")

foreach(SHADER_FILE ${SHADER_FILES})
    # 获取文件名（不含扩展名）和扩展名
    get_filename_component(SHADER_NAME ${SHADER_FILE} NAME_WE)
    get_filename_component(SHADER_EXT ${SHADER_FILE} EXT)

    # 清理扩展名（如 .frag → _frag）
    string(REPLACE "." "_" EXT_CLEAN ${SHADER_EXT})
    set(OUTPUT_NAME "${SHADER_NAME}${EXT_CLEAN}.spv")

    set(SHADER_SOURCE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/${SHADER_FILE})
    set(SHADER_OUTPUT_PATH ${SHADER_OUTPUT_DIR}/${OUTPUT_NAME})

    # 为每个着色器创建自定义构建命令
    add_custom_command(
        OUTPUT ${SHADER_OUTPUT_PATH}
        COMMAND ${GLSLC_PROGRAM} ${SHADER_SOURCE_PATH} -o ${SHADER_OUTPUT_PATH}
        DEPENDS ${SHADER_SOURCE_PATH}
        COMMENT "Compiling shader: ${SHADER_FILE} -> ${OUTPUT_NAME}"
        VERBATIM
    )

    # 收集输出文件，用于 add_custom_target
    list(APPEND SHADER_OUTPUTS ${SHADER_OUTPUT_PATH})
endforeach()

# 创建一个统一的 target，依赖所有着色器输出
add_custom_target(build_shaders ALL
    DEPENDS
    ${SHADER_OUTPUTS}
    COMMENT "Building all shaders"

)
