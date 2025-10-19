cmake_minimum_required(VERSION 3.16)
project(ShaderCompiler)

include(BuildShader.cmake)

set(SHADER_PATH "data/shader/")
set(SHADER_EXTENSION ".spv")
set(SHADER_PATH_INCLUDE ${CMAKE_CURRENT_BINARY_DIR}/include PARENT_SCOPE)
set(SHADER_INCLUDE ${CMAKE_CURRENT_BINARY_DIR}/include)
# 配置模板
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/source_shader.h.in"
    "${SHADER_INCLUDE}/source_shader.h"
    @ONLY
)
