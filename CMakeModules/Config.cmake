# 设置默认值

# 根据构建类型设置值
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(VULKAN_VALIDATION "true")
else()
    set(VULKAN_VALIDATION "false")
endif()

# 使用 configure_file 生成最终的 config.yaml
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/config/config.yaml.in"
    "${CMAKE_BINARY_DIR}/bin/config/config.yaml"
    @ONLY
)
