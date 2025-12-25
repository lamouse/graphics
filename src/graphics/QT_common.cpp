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
#ifdef _WIN32
    wsi.render_surface_scale = window ? window->screen()->logicalDotsPerInch() / 96.0f : 1.f;
#else
    wsi.render_surface_scale = window ? static_cast<float>(window->devicePixelRatio()) : 1.0f;
#endif
    return wsi;
}

auto QtButtonToMouseButton(Qt::MouseButton button) -> input::MouseButton {
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

auto KeyConvert(Qt::KeyboardModifiers qt_modifiers) -> input::NativeKeyboard::Modifiers {
    input::NativeKeyboard::Modifiers modifiers = input::NativeKeyboard::Modifiers::None;
    if ((qt_modifiers & Qt::KeyboardModifier::ShiftModifier) != 0) {
        modifiers |= input::NativeKeyboard::Modifiers::LeftShift;
    }
    if ((qt_modifiers & Qt::KeyboardModifier::ControlModifier) != 0) {
        modifiers |= input::NativeKeyboard::Modifiers::LeftControl;
    }
    if ((qt_modifiers & Qt::KeyboardModifier::AltModifier) != 0) {
        modifiers |= input::NativeKeyboard::Modifiers::LeftAlt;
    }
    if ((qt_modifiers & Qt::KeyboardModifier::MetaModifier) != 0) {
        modifiers |= input::NativeKeyboard::Modifiers::LeftMeta;
    }
    return modifiers;
}

auto QtKeyToSwitchKey(Qt::Key qt_key) -> input::NativeKeyboard::Keys {
    static constexpr std::array<std::pair<Qt::Key, input::NativeKeyboard::Keys>, 106> key_map = {
        std::pair<Qt::Key, input::NativeKeyboard::Keys>{Qt::Key_A, input::NativeKeyboard::Keys::A},
        {Qt::Key_B, input::NativeKeyboard::Keys::B},
        {Qt::Key_C, input::NativeKeyboard::Keys::C},
        {Qt::Key_D, input::NativeKeyboard::Keys::D},
        {Qt::Key_E, input::NativeKeyboard::Keys::E},
        {Qt::Key_F, input::NativeKeyboard::Keys::F},
        {Qt::Key_G, input::NativeKeyboard::Keys::G},
        {Qt::Key_H, input::NativeKeyboard::Keys::H},
        {Qt::Key_I, input::NativeKeyboard::Keys::I},
        {Qt::Key_J, input::NativeKeyboard::Keys::J},
        {Qt::Key_K, input::NativeKeyboard::Keys::K},
        {Qt::Key_L, input::NativeKeyboard::Keys::L},
        {Qt::Key_M, input::NativeKeyboard::Keys::M},
        {Qt::Key_N, input::NativeKeyboard::Keys::N},
        {Qt::Key_O, input::NativeKeyboard::Keys::O},
        {Qt::Key_P, input::NativeKeyboard::Keys::P},
        {Qt::Key_Q, input::NativeKeyboard::Keys::Q},
        {Qt::Key_R, input::NativeKeyboard::Keys::R},
        {Qt::Key_S, input::NativeKeyboard::Keys::S},
        {Qt::Key_T, input::NativeKeyboard::Keys::T},
        {Qt::Key_U, input::NativeKeyboard::Keys::U},
        {Qt::Key_V, input::NativeKeyboard::Keys::V},
        {Qt::Key_W, input::NativeKeyboard::Keys::W},
        {Qt::Key_X, input::NativeKeyboard::Keys::X},
        {Qt::Key_Y, input::NativeKeyboard::Keys::Y},
        {Qt::Key_Z, input::NativeKeyboard::Keys::Z},
        {Qt::Key_1, input::NativeKeyboard::Keys::N1},
        {Qt::Key_2, input::NativeKeyboard::Keys::N2},
        {Qt::Key_3, input::NativeKeyboard::Keys::N3},
        {Qt::Key_4, input::NativeKeyboard::Keys::N4},
        {Qt::Key_5, input::NativeKeyboard::Keys::N5},
        {Qt::Key_6, input::NativeKeyboard::Keys::N6},
        {Qt::Key_7, input::NativeKeyboard::Keys::N7},
        {Qt::Key_8, input::NativeKeyboard::Keys::N8},
        {Qt::Key_9, input::NativeKeyboard::Keys::N9},
        {Qt::Key_0, input::NativeKeyboard::Keys::N0},
        {Qt::Key_Return, input::NativeKeyboard::Keys::Return},
        {Qt::Key_Escape, input::NativeKeyboard::Keys::Escape},
        {Qt::Key_Backspace, input::NativeKeyboard::Keys::Backspace},
        {Qt::Key_Tab, input::NativeKeyboard::Keys::Tab},
        {Qt::Key_Space, input::NativeKeyboard::Keys::Space},
        {Qt::Key_Minus, input::NativeKeyboard::Keys::Minus},
        {Qt::Key_Plus, input::NativeKeyboard::Keys::Plus},
        {Qt::Key_questiondown, input::NativeKeyboard::Keys::Plus},
        {Qt::Key_BracketLeft, input::NativeKeyboard::Keys::OpenBracket},
        {Qt::Key_BraceLeft, input::NativeKeyboard::Keys::OpenBracket},
        {Qt::Key_BracketRight, input::NativeKeyboard::Keys::CloseBracket},
        {Qt::Key_BraceRight, input::NativeKeyboard::Keys::CloseBracket},
        {Qt::Key_Bar, input::NativeKeyboard::Keys::Pipe},
        {Qt::Key_Dead_Tilde, input::NativeKeyboard::Keys::Tilde},
        {Qt::Key_Ntilde, input::NativeKeyboard::Keys::Semicolon},
        {Qt::Key_Semicolon, input::NativeKeyboard::Keys::Semicolon},
        {Qt::Key_Apostrophe, input::NativeKeyboard::Keys::Quote},
        {Qt::Key_Dead_Grave, input::NativeKeyboard::Keys::Backquote},
        {Qt::Key_Comma, input::NativeKeyboard::Keys::Comma},
        {Qt::Key_Period, input::NativeKeyboard::Keys::Period},
        {Qt::Key_Slash, input::NativeKeyboard::Keys::Slash},
        {Qt::Key_CapsLock, input::NativeKeyboard::Keys::CapsLockKey},
        {Qt::Key_F1, input::NativeKeyboard::Keys::F1},
        {Qt::Key_F2, input::NativeKeyboard::Keys::F2},
        {Qt::Key_F3, input::NativeKeyboard::Keys::F3},
        {Qt::Key_F4, input::NativeKeyboard::Keys::F4},
        {Qt::Key_F5, input::NativeKeyboard::Keys::F5},
        {Qt::Key_F6, input::NativeKeyboard::Keys::F6},
        {Qt::Key_F7, input::NativeKeyboard::Keys::F7},
        {Qt::Key_F8, input::NativeKeyboard::Keys::F8},
        {Qt::Key_F9, input::NativeKeyboard::Keys::F9},
        {Qt::Key_F10, input::NativeKeyboard::Keys::F10},
        {Qt::Key_F11, input::NativeKeyboard::Keys::F11},
        {Qt::Key_F12, input::NativeKeyboard::Keys::F12},
        {Qt::Key_Print, input::NativeKeyboard::Keys::PrintScreen},
        {Qt::Key_ScrollLock, input::NativeKeyboard::Keys::ScrollLockKey},
        {Qt::Key_Pause, input::NativeKeyboard::Keys::Pause},
        {Qt::Key_Insert, input::NativeKeyboard::Keys::Insert},
        {Qt::Key_Home, input::NativeKeyboard::Keys::Home},
        {Qt::Key_PageUp, input::NativeKeyboard::Keys::PageUp},
        {Qt::Key_Delete, input::NativeKeyboard::Keys::Delete},
        {Qt::Key_End, input::NativeKeyboard::Keys::End},
        {Qt::Key_PageDown, input::NativeKeyboard::Keys::PageDown},
        {Qt::Key_Right, input::NativeKeyboard::Keys::Right},
        {Qt::Key_Left, input::NativeKeyboard::Keys::Left},
        {Qt::Key_Down, input::NativeKeyboard::Keys::Down},
        {Qt::Key_Up, input::NativeKeyboard::Keys::Up},
        {Qt::Key_NumLock, input::NativeKeyboard::Keys::NumLockKey},
        // Numpad keys are missing here
        {Qt::Key_F13, input::NativeKeyboard::Keys::F13},
        {Qt::Key_F14, input::NativeKeyboard::Keys::F14},
        {Qt::Key_F15, input::NativeKeyboard::Keys::F15},
        {Qt::Key_F16, input::NativeKeyboard::Keys::F16},
        {Qt::Key_F17, input::NativeKeyboard::Keys::F17},
        {Qt::Key_F18, input::NativeKeyboard::Keys::F18},
        {Qt::Key_F19, input::NativeKeyboard::Keys::F19},
        {Qt::Key_F20, input::NativeKeyboard::Keys::F20},
        {Qt::Key_F21, input::NativeKeyboard::Keys::F21},
        {Qt::Key_F22, input::NativeKeyboard::Keys::F22},
        {Qt::Key_F23, input::NativeKeyboard::Keys::F23},
        {Qt::Key_F24, input::NativeKeyboard::Keys::F24},
        // {Qt::..., input::NativeKeyboard::KPComma},
        // {Qt::..., input::NativeKeyboard::Ro},
        {Qt::Key_Hiragana_Katakana, input::NativeKeyboard::Keys::KatakanaHiragana},
        {Qt::Key_yen, input::NativeKeyboard::Keys::Yen},
        {Qt::Key_Henkan, input::NativeKeyboard::Keys::Henkan},
        {Qt::Key_Muhenkan, input::NativeKeyboard::Keys::Muhenkan},
        // {Qt::..., input::NativeKeyboard::NumPadCommaPc98},
        {Qt::Key_Hangul, input::NativeKeyboard::Keys::HangulEnglish},
        {Qt::Key_Hangul_Hanja, input::NativeKeyboard::Keys::Hanja},
        {Qt::Key_Katakana, input::NativeKeyboard::Keys::KatakanaKey},
        {Qt::Key_Hiragana, input::NativeKeyboard::Keys::HiraganaKey},
        {Qt::Key_Zenkaku_Hankaku, input::NativeKeyboard::Keys::ZenkakuHankaku},
        // Modifier keys are handled by the modifier property
    };

    for (const auto& [qkey, nkey] : key_map) {
        if (qt_key == qkey) {
            return nkey;
        }
    }

    return input::NativeKeyboard::Keys::None;
}
}  // namespace graphics::qt