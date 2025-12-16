set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(PROGRAM_NAME "graphics")

include(${CMAKE_CURRENT_SOURCE_DIR}/graphics.cmake)

add_executable(${PROGRAM_NAME} ${sources})
target_compile_definitions(${PROGRAM_NAME} PRIVATE USE_SDL)

add_dependencies(${PROGRAM_NAME} copy_fonts)

# target_compile_definitions(${PROGRAM_NAME} PRIVATE USE_GLFW)
target_link_libraries(${PROGRAM_NAME} PRIVATE core utils render-core resource)
target_link_libraries(${PROGRAM_NAME} PRIVATE glfw spdlog::spdlog)
target_link_libraries(${PROGRAM_NAME} PRIVATE effects Qt${QT_VERSION_MAJOR}::Widgets)
target_link_libraries(${PROGRAM_NAME} PRIVATE SDL3::SDL3 world system Imgui::Imgui ImGuizmo::ImGuizmo)

target_link_directories(${PROGRAM_NAME} PRIVATE ${CMAKE_BINARY_DIR}/lib)

add_custom_command(TARGET ${PROGRAM_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_RUNTIME_DLLS:${PROGRAM_NAME}>
    $<TARGET_FILE_DIR:${PROGRAM_NAME}>
    COMMAND_EXPAND_LISTS
)

if(LINUX)
    target_link_libraries(
        ${PROGRAM_NAME} PRIVATE
        X11 # 🔴 必须！解决 XInternAtom, XChangeProperty 等
        Xrandr # 推荐：多屏支持
        Xinerama # 推荐：虚拟桌面拼接检测
        Xcursor # 推荐：鼠标光标
        Xxf86vm # 推荐：全屏切换
        Xi # 推荐：输入设备扩展
    )
endif()

if(WIN32 AND NOT CMAKE_CROSSCOMPILING)
    # 获取 Qt6 安装前缀（vcpkg 会设置 Qt6_DIR）
    get_target_property(_qt6_core_location Qt6::Core LOCATION)
    get_filename_component(_qt6_bin_dir "${_qt6_core_location}" DIRECTORY)
    get_filename_component(_qt6_prefix "${_qt6_bin_dir}/.." ABSOLUTE)
    set(QT6_PLUGINS_DIR "${_qt6_prefix}/Qt6/plugins/platforms/qwindows.dll")
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(QT6_PLUGINS_DIR "${_qt6_prefix}/debug/Qt6/plugins/platforms/qwindowsd.dll")
    endif()
    # 构建后复制平台插件 qwindows.dll
    add_custom_command(TARGET ${PROGRAM_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROGRAM_NAME}>/plugins/platforms"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${QT6_PLUGINS_DIR}
        "$<TARGET_FILE_DIR:${PROGRAM_NAME}>/plugins/platforms/"
        COMMENT "Copying Qt platform plugin qwindows.dll..."
    )
endif()

# 添加生成的代码目录
include_directories(${CMAKE_BINARY_DIR}/generated)

if(MSVC)
    set_target_properties(${PROGRAM_NAME} PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS")
elseif(MINGW)
    set_target_properties(${PROGRAM_NAME} PROPERTIES LINK_FLAGS_RELEASE "-Wl,--subsystem,windows")
endif()