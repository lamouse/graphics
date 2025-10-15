cmake_minimum_required(VERSION 3.12)
# 创建一个 INTERFACE 库目标
add_library(stb INTERFACE)

# 设置头文件包含路径
target_include_directories(stb INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)