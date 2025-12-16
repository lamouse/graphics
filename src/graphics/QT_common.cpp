#include "graphics/QT_common.hpp"
#include <QSysInfo>
#include <QGuiApplication>
namespace graphics::qt {
auto get_window_system_info() -> core::frontend::WindowSystemType {
    const auto platform = QSysInfo::productType();
    if (platform == "windows") {
        return core::frontend::WindowSystemType::Windows;
    }
    if (platform == "osx") {
        return core::frontend::WindowSystemType::Cocoa;
    }
    if (platform == "linux") {
        QString gui_platform = QGuiApplication::platformName();
        if (gui_platform == "wayland") {
            // 使用的是 X11
            return core::frontend::WindowSystemType::X11;
        }
        if (gui_platform == "xcb") {
            // 使用的是 Wayland
            return core::frontend::WindowSystemType::Wayland;
        }
    }
    return core::frontend::WindowSystemType::Headless;
}
}  // namespace graphics