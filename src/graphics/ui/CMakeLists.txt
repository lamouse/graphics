set(MODULE_NAME "RenderCtrl")
set(LIB_NAME "${MODULE_NAME}Lib")
qt_add_library(${LIB_NAME} STATIC)
set_target_properties(${LIB_NAME} PROPERTIES
    AUTOMOC ON)

qt_add_qml_module(${LIB_NAME}
    URI ${MODULE_NAME}

    # VERSION 1.0
    RESOURCE_PREFIX /
    QML_FILES
    RenderControl.qml
    SOURCES
    render_config.hpp
    render_config.cpp
    scaling_filter.hpp
    OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/qml/RenderCtrl"
)
target_link_libraries(${LIB_NAME} PRIVATE common Imgui::Imgui)
target_link_libraries(${LIB_NAME} PUBLIC
    Qt6::Quick Qt6::QuickControls2 Qt6::QuickControls2Basic Qt6::QuickControls2Fusion Qt6::QuickDialogs2 Qt6::QuickEffects
    Qt6::QuickControls2WindowsStyleImpl Qt6::QuickLayouts
)
