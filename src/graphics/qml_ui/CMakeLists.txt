set(QML_MODULE_NAME "qml_ui")
set(LIB_NAME "${QML_MODULE_NAME}")
qt_add_library(${LIB_NAME} STATIC)
set_target_properties(${LIB_NAME} PROPERTIES
    AUTOMOC ON)

qt_add_qml_module(${QML_MODULE_NAME}
    URI ${QML_MODULE_NAME}
    # VERSION 1.0
    RESOURCE_PREFIX /
    QML_FILES
        RenderControl.qml
)

target_link_libraries(${LIB_NAME} PUBLIC
    Qt6::Quick Qt6::QuickControls2 Qt6::QuickControls2Basic Qt6::QuickControls2Fusion Qt6::QuickDialogs2 Qt6::QuickEffects
    Qt6::QuickControls2WindowsStyleImpl
)
