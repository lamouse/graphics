set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(PROGRAM_NAME "graphics")

include(${CMAKE_CURRENT_SOURCE_DIR}/graphics.cmake)


add_executable(${PROGRAM_NAME} ${sources})

add_dependencies(${PROGRAM_NAME} copy_fonts)


target_link_libraries(${PROGRAM_NAME} PRIVATE core utils render-core effects resource input ImGuiFileDialog::ImGuiFileDialog)
target_link_libraries(${PROGRAM_NAME} PRIVATE SDL3::SDL3 world system Imgui::Imgui ImGuizmo::ImGuizmo)

target_link_directories(${PROGRAM_NAME} PRIVATE ${CMAKE_BINARY_DIR}/lib)

if(WIN32)
add_custom_command(TARGET ${PROGRAM_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_RUNTIME_DLLS:${PROGRAM_NAME}>
    $<TARGET_FILE_DIR:${PROGRAM_NAME}>
    COMMAND_EXPAND_LISTS
)
endif()

#if(LINUX)
#    target_link_libraries(
#        ${PROGRAM_NAME} PRIVATE
#        X11 # 🔴 必须！解决 XInternAtom, XChangeProperty 等
#        Xrandr # 推荐：多屏支持
#        Xinerama # 推荐：虚拟桌面拼接检测
#        Xcursor # 推荐：鼠标光标
#        Xxf86vm # 推荐：全屏切换
#        Xi # 推荐：输入设备扩展
#    )
#endif()

# 添加生成的代码目录
include_directories(${CMAKE_BINARY_DIR}/generated)

if(MSVC)
    set_target_properties(${PROGRAM_NAME} PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS")
elseif(MINGW)
    set_target_properties(${PROGRAM_NAME} PROPERTIES LINK_FLAGS_RELEASE "-Wl,--subsystem,windows")
endif()