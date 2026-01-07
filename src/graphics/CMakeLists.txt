set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(PROGRAM_NAME "graphics")
set(QML_MODULE_NAME "graphics")
set(QML_IMPORT_PATH "${CMAKE_BINARY_DIR}/qml" CACHE STRING "QML import path")

include(${CMAKE_CURRENT_SOURCE_DIR}/graphics.cmake)

if(USE_QT AND TARGET Qt6::Quick)
    add_subdirectory(ui)
    qt_standard_project_setup(REQUIRES 6.8)
    qt_add_executable(${PROGRAM_NAME} ${sources})
    set_target_properties(${PROGRAM_NAME} PROPERTIES
        AUTOUIC ON
        AUTOMOC ON
        AUTORCC ON
    )
    qt_add_qml_module(${PROGRAM_NAME}
        URI ${QML_MODULE_NAME}
        VERSION 1.0
        QML_FILES
        Main.qml
    )
else()
    add_executable(${PROGRAM_NAME} ${sources})
endif()

add_dependencies(${PROGRAM_NAME} copy_fonts)

if(USE_QT)
    target_compile_definitions(${PROGRAM_NAME} PRIVATE USE_QT)

    if(WIN32 AND NOT CMAKE_CROSSCOMPILING)
        get_target_property(_windeployqt_exe Qt6::windeployqt IMPORTED_LOCATION)

        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            if(USE_VCPKG)
                string(REPLACE "windeployqt.exe" "windeployqt.debug.bat" _windeployqt_exe "${_windeployqt_exe}")
            endif()
        endif()

        # 构建后复制平台插件 qwindows.dll
        add_custom_command(TARGET ${PROGRAM_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROGRAM_NAME}>/plugins/platforms"
            COMMAND ${_windeployqt_exe}
            --include-soft-plugins
            --qmldir ${CMAKE_CURRENT_SOURCE_DIR}
            --no-translations
            # --no-libraries
            --skip-plugin-types networkinformation,imageformats,iconengines,qmltooling,styles,tls,generic
            "$<TARGET_FILE_DIR:${PROGRAM_NAME}>"
            COMMENT "windeployqt running..."
        )
    endif()

    if(TARGET Qt6::Quick)
        target_link_libraries(${PROGRAM_NAME} PRIVATE RenderCtrlLibplugin)
    endif()

    target_link_libraries(${PROGRAM_NAME} PRIVATE Qt6::Widgets)

else()
    target_compile_definitions(${PROGRAM_NAME} PRIVATE USE_SDL)
endif()

target_link_libraries(${PROGRAM_NAME} PRIVATE core utils render-core effects resource input)
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

if(MSVC)
    set_target_properties(${PROGRAM_NAME} PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS")
elseif(MINGW)
    set_target_properties(${PROGRAM_NAME} PROPERTIES LINK_FLAGS_RELEASE "-Wl,--subsystem,windows")
endif()