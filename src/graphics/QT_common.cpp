#include "graphics/QT_common.hpp"
#include <QSysInfo>
#include <QGuiApplication>
#include <QWindow>
#include <spdlog/spdlog.h>
#include "input/mouse.h"

#if !defined(WIN32) && !defined(__APPLE__)
#include <qpa/qplatformnativeinterface.h>
#elif defined(__APPLE__)
#include <objc/message.h>
#endif
namespace graphics::qt {

auto get_window_system_type() -> core::frontend::WindowSystemType {
    // Determine WSI type based on Qt platform.
    QString platform_name = QGuiApplication::platformName();
    if (platform_name == QStringLiteral("windows")) {
        return core::frontend::WindowSystemType::Windows;
    }
    if (platform_name == QStringLiteral("xcb")) {
        return core::frontend::WindowSystemType::X11;
    }
    if (platform_name == QStringLiteral("wayland")) {
        return core::frontend::WindowSystemType::Wayland;
    }
    if (platform_name == QStringLiteral("wayland-egl")) {
        return core::frontend::WindowSystemType::Wayland;
    }
    if (platform_name == QStringLiteral("cocoa")) {
        return core::frontend::WindowSystemType::Cocoa;
    }
    if (platform_name == QStringLiteral("android")) {
        return core::frontend::WindowSystemType::Android;
    }
    if (platform_name == QStringLiteral("haiku")) {
        return core::frontend::WindowSystemType::Xcb;
    }

    spdlog::warn("Unknown Qt platform {}!", platform_name.toStdString());
    return core::frontend::WindowSystemType::Headless;
}
auto get_window_system_info(QWindow* window) -> core::frontend::BaseWindow::WindowSystemInfo {
    core::frontend::BaseWindow::WindowSystemInfo wsi{};
    wsi.type = get_window_system_type();
#if defined(WIN32)
    // Our Win32 Qt external doesn't have the private API.
    wsi.render_surface = reinterpret_cast<void*>(window->winId());
#elif defined(__APPLE__)
    wsi.render_surface = reinterpret_cast<void* (*)(id, SEL)>(objc_msgSend)(
        reinterpret_cast<id>(window->winId()), sel_registerName("layer"));
#else
    QPlatformNativeInterface* pni = QGuiApplication::platformNativeInterface();
    wsi.display_connection = pni->nativeResourceForWindow("display", window);
    if (wsi.type == Core::Frontend::WindowSystemType::Wayland)
        wsi.render_surface = window ? pni->nativeResourceForWindow("surface", window) : nullptr;
    else
        wsi.render_surface = window ? reinterpret_cast<void*>(window->winId()) : nullptr;
#endif
    wsi.render_surface_scale = window ? static_cast<float>(window->devicePixelRatio()) : 1.0f;
    return wsi;
}

auto QtButtonToMouseButton(Qt::MouseButton button) -> input::MouseButton{
    switch (button) {
    case Qt::LeftButton:
        return input::MouseButton::Left;
    case Qt::RightButton:
        return input::MouseButton::Right;
    case Qt::MiddleButton:
        return input::MouseButton::Middle;
    case Qt::BackButton:
        return input::MouseButton::Backward;
    case Qt::ForwardButton:
        return input::MouseButton::Forward;
    default:
        return input::MouseButton::UnDefined;
    }
}
}  // namespace graphics::qt