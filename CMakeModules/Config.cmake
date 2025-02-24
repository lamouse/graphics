# 设置默认值

# 根据构建类型设置值
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(VULKAN_VALIDATION "true")
    set(LOG_LEVEL "debug")
    set(LGO_CONSOLE_ENABLE "true")
    set(LGO_FILE_ENABLE "false")
    #-DUSE_DEBUG_UI 用来显示imgui debug窗口
    add_definitions(-DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG -DUSE_DEBUG_UI)

else()
    set(VULKAN_VALIDATION "false")
    set(LGO_CONSOLE_ENABLE "false")
    set(LGO_FILE_ENABLE "true")
    set(LOG_LEVEL "warn")
    add_definitions(-DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_WARN)

endif()
#设置使用的库
add_definitions(-DUSE_SDL)
#add_definitions(-DUSE_GLFW)

set(WINDOW_TITLE "${PROJECT_NAME}")
# 使用 configure_file 生成最终的 config.yaml
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/config/config.yaml.in"
    "${CMAKE_BINARY_DIR}/bin/config/config.yaml"
    @ONLY
)

set(CONFIG_OUTPUT_DIR "${CMAKE_BINARY_DIR}/generated/config")
find_package(Python3 REQUIRED)
if(Python3_EXECUTABLE)
    execute_process(COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/config/generate_cpp_files.py ${CMAKE_BINARY_DIR}/bin/config ${CONFIG_OUTPUT_DIR}
    RESULT_VARIABLE result)
    if(NOT result EQUAL "0")
        message(FATAL_ERROR "Command failed with error: ${result}")
    endif()
else()
    message(FATAL_ERROR "python3 not found")
endif()
