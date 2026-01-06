#include "SDL_common.hpp"
#include <cstring>
#ifdef __linux__
#include <X11/Xlib.h>
#endif
namespace graphics {
auto get_window_system_info() -> core::frontend::WindowSystemType {
    const auto *platform = SDL_GetPlatform();
    if (strcmp(platform, "Windows") == 0) {
        return core::frontend::WindowSystemType::Windows;
    }
    if (strcmp(platform, "Mac OS X") == 0) {
        return core::frontend::WindowSystemType::Cocoa;
    }
    if (strcmp(platform, "Linux") == 0) {
        const char *video_driver = SDL_GetCurrentVideoDriver();
        if (strcmp(video_driver, "x11") == 0) {
            // 使用的是 X11
            return core::frontend::WindowSystemType::X11;
        }
        if (strcmp(video_driver, "wayland") == 0) {
            // 使用的是 Wayland
            return core::frontend::WindowSystemType::Wayland;
        }
    }
    return core::frontend::WindowSystemType::Headless;
}

auto get_windows_handles(SDL_Window *window) -> void * {
#if defined(SDL_PLATFORM_WIN32)
    return SDL_GetPointerProperty(SDL_GetWindowProperties(window),
                                  SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);

#elif defined(SDL_PLATFORM_MACOS)
    NSWindow *nswindow = (__bridge NSWindow *)SDL_GetPointerProperty(
        SDL_GetWindowProperties(window), SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, NULL);
    if (nswindow) {
        return nswindow;
    }
    return nullptr;
#elif defined(SDL_PLATFORM_LINUX)
    if (SDL_strcmp(SDL_GetCurrentVideoDriver(), "x11") == 0) {
        return reinterpret_cast<void *>(SDL_GetNumberProperty(
            SDL_GetWindowProperties(window), SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0));
    }
    if (SDL_strcmp(SDL_GetCurrentVideoDriver(), "wayland") == 0) {
        return SDL_GetPointerProperty(SDL_GetWindowProperties(window),
                                      SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, nullptr);
    }
#endif
    return nullptr;
}

#ifdef __linux__
auto get_windows_display(SDL_Window *window) -> void * {
#if defined(SDL_PLATFORM_LINUX)
    if (SDL_strcmp(SDL_GetCurrentVideoDriver(), "x11") == 0) {
        return SDL_GetPointerProperty(SDL_GetWindowProperties(window),
                                      SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr);
    }
    if (SDL_strcmp(SDL_GetCurrentVideoDriver(), "wayland") == 0) {
        return SDL_GetPointerProperty(SDL_GetWindowProperties(window),
                                      SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, nullptr);
    }
#endif
    return nullptr;
}

#endif

auto get_window_info(SDL_Window *window) -> core::frontend::BaseWindow::WindowSystemInfo {
    core::frontend::BaseWindow::WindowSystemInfo window_info;
#if defined(SDL_PLATFORM_MACOS)
    window_info.render_surface_scale = 1.0f;
#else
    window_info.render_surface_scale = SDL_GetWindowDisplayScale(window);
#endif
    window_info.type = get_window_system_info();
    window_info.render_surface = get_windows_handles(window);
#ifdef __linux__
    window_info.display_connection = get_windows_display(window_);
#endif
    return window_info;
}

auto FromSDLKeymod(SDL_Keymod sdlMod) -> input::NativeKeyboard::Modifiers {
    input::NativeKeyboard::Modifiers result = input::NativeKeyboard::Modifiers::None;
    if (sdlMod & SDL_KMOD_LCTRL) result |= input::NativeKeyboard::Modifiers::LeftControl;
    if (sdlMod & SDL_KMOD_RCTRL) result |= input::NativeKeyboard::Modifiers::RightControl;
    if (sdlMod & SDL_KMOD_LSHIFT) result |= input::NativeKeyboard::Modifiers::LeftShift;
    if (sdlMod & SDL_KMOD_RSHIFT) result |= input::NativeKeyboard::Modifiers::RightShift;
    if (sdlMod & SDL_KMOD_LALT) result |= input::NativeKeyboard::Modifiers::LeftAlt;
    if (sdlMod & SDL_KMOD_RALT) result |= input::NativeKeyboard::Modifiers::RightAlt;
    if (sdlMod & SDL_KMOD_LGUI) result |= input::NativeKeyboard::Modifiers::LeftMeta;
    if (sdlMod & SDL_KMOD_RGUI) result |= input::NativeKeyboard::Modifiers::RightMeta;
    if (sdlMod & SDL_KMOD_CAPS) result |= input::NativeKeyboard::Modifiers::CapsLock;
    if (sdlMod & SDL_KMOD_SCROLL) result |= input::NativeKeyboard::Modifiers::ScrollLock;
    if (sdlMod & SDL_KMOD_NUM) result |= input::NativeKeyboard::Modifiers::NumLock;

    return result;
}
auto FromSDLKeycode(SDL_Keycode code) -> input::NativeKeyboard::Keys {
    using namespace input::NativeKeyboard;
    switch (code) {
        case SDLK_A:
            return Keys::A;
        case SDLK_B:
            return Keys::B;
        case SDLK_C:
            return Keys::C;
        case SDLK_D:
            return Keys::D;
        case SDLK_E:
            return Keys::E;
        case SDLK_F:
            return Keys::F;
        case SDLK_G:
            return Keys::G;
        case SDLK_H:
            return Keys::H;
        case SDLK_I:
            return Keys::I;
        case SDLK_J:
            return Keys::J;
        case SDLK_K:
            return Keys::K;
        case SDLK_L:
            return Keys::L;
        case SDLK_M:
            return Keys::M;
        case SDLK_N:
            return Keys::N;
        case SDLK_O:
            return Keys::O;
        case SDLK_P:
            return Keys::P;
        case SDLK_Q:
            return Keys::Q;
        case SDLK_R:
            return Keys::R;
        case SDLK_S:
            return Keys::S;
        case SDLK_T:
            return Keys::T;
        case SDLK_U:
            return Keys::U;
        case SDLK_V:
            return Keys::V;
        case SDLK_W:
            return Keys::W;
        case SDLK_X:
            return Keys::X;
        case SDLK_Y:
            return Keys::Y;
        case SDLK_Z:
            return Keys::Z;

        case SDLK_1:
            return Keys::N1;
        case SDLK_2:
            return Keys::N2;
        case SDLK_3:
            return Keys::N3;
        case SDLK_4:
            return Keys::N4;
        case SDLK_5:
            return Keys::N5;
        case SDLK_6:
            return Keys::N6;
        case SDLK_7:
            return Keys::N7;
        case SDLK_8:
            return Keys::N8;
        case SDLK_9:
            return Keys::N9;
        case SDLK_0:
            return Keys::N0;

        case SDLK_RETURN:
            return Keys::Return;
        case SDLK_ESCAPE:
            return Keys::Escape;
        case SDLK_BACKSPACE:
            return Keys::Backspace;
        case SDLK_TAB:
            return Keys::Tab;
        case SDLK_SPACE:
            return Keys::Space;
        case SDLK_MINUS:
            return Keys::Minus;
        case SDLK_EQUALS:
            return Keys::Plus;  // 注意：SDL 中 '+' 是 Equals
        case SDLK_LEFTBRACKET:
            return Keys::OpenBracket;
        case SDLK_RIGHTBRACKET:
            return Keys::CloseBracket;
        case SDLK_BACKSLASH:
            return Keys::Pipe;
        case SDLK_SEMICOLON:
            return Keys::Semicolon;
        case SDLK_APOSTROPHE:
            return Keys::Quote;
        case SDLK_GRAVE:
            return Keys::Backquote;
        case SDLK_COMMA:
            return Keys::Comma;
        case SDLK_PERIOD:
            return Keys::Period;
        case SDLK_SLASH:
            return Keys::Slash;

        case SDLK_CAPSLOCK:
            return Keys::CapsLockKey;

        case SDLK_F1:
            return Keys::F1;
        case SDLK_F2:
            return Keys::F2;
        case SDLK_F3:
            return Keys::F3;
        case SDLK_F4:
            return Keys::F4;
        case SDLK_F5:
            return Keys::F5;
        case SDLK_F6:
            return Keys::F6;
        case SDLK_F7:
            return Keys::F7;
        case SDLK_F8:
            return Keys::F8;
        case SDLK_F9:
            return Keys::F9;
        case SDLK_F10:
            return Keys::F10;
        case SDLK_F11:
            return Keys::F11;
        case SDLK_F12:
            return Keys::F12;
        case SDLK_F13:
            return Keys::F13;
        case SDLK_F14:
            return Keys::F14;
        case SDLK_F15:
            return Keys::F15;
        case SDLK_F16:
            return Keys::F16;
        case SDLK_F17:
            return Keys::F17;
        case SDLK_F18:
            return Keys::F18;
        case SDLK_F19:
            return Keys::F19;
        case SDLK_F20:
            return Keys::F20;
        case SDLK_F21:
            return Keys::F21;
        case SDLK_F22:
            return Keys::F22;
        case SDLK_F23:
            return Keys::F23;
        case SDLK_F24:
            return Keys::F24;

        case SDLK_PRINTSCREEN:
            return Keys::PrintScreen;
        case SDLK_SCROLLLOCK:
            return Keys::ScrollLockKey;
        case SDLK_PAUSE:
            return Keys::Pause;
        case SDLK_INSERT:
            return Keys::Insert;
        case SDLK_HOME:
            return Keys::Home;
        case SDLK_PAGEUP:
            return Keys::PageUp;
        case SDLK_DELETE:
            return Keys::Delete;
        case SDLK_END:
            return Keys::End;
        case SDLK_PAGEDOWN:
            return Keys::PageDown;
        case SDLK_RIGHT:
            return Keys::Right;
        case SDLK_LEFT:
            return Keys::Left;
        case SDLK_DOWN:
            return Keys::Down;
        case SDLK_UP:
            return Keys::Up;

        case SDLK_NUMLOCKCLEAR:
            return Keys::NumLockKey;
        case SDLK_KP_DIVIDE:
            return Keys::KPSlash;
        case SDLK_KP_MULTIPLY:
            return Keys::KPAsterisk;
        case SDLK_KP_MINUS:
            return Keys::KPMinus;
        case SDLK_KP_PLUS:
            return Keys::KPPlus;
        case SDLK_KP_ENTER:
            return Keys::KPEnter;
        case SDLK_KP_1:
            return Keys::KP1;
        case SDLK_KP_2:
            return Keys::KP2;
        case SDLK_KP_3:
            return Keys::KP3;
        case SDLK_KP_4:
            return Keys::KP4;
        case SDLK_KP_5:
            return Keys::KP5;
        case SDLK_KP_6:
            return Keys::KP6;
        case SDLK_KP_7:
            return Keys::KP7;
        case SDLK_KP_8:
            return Keys::KP8;
        case SDLK_KP_9:
            return Keys::KP9;
        case SDLK_KP_0:
            return Keys::KP0;
        case SDLK_KP_PERIOD:
            return Keys::KPDot;
        case SDLK_KP_EQUALS:
            return Keys::KPEqual;
        case SDLK_KP_COMMA:
            return Keys::KPComma;

        case SDLK_APPLICATION:
            return Keys::Compose;  // 常用于 Compose 键
        case SDLK_POWER:
            return Keys::Power;

        case SDLK_EXECUTE:
            return Keys::Open;
        case SDLK_HELP:
            return Keys::Help;
        case SDLK_MENU:
            return Keys::Properties;
        case SDLK_STOP:
            return Keys::Stop;
        case SDLK_AGAIN:
            return Keys::Repeat;
        case SDLK_UNDO:
            return Keys::Undo;
        case SDLK_CUT:
            return Keys::Cut;
        case SDLK_COPY:
            return Keys::Copy;
        case SDLK_PASTE:
            return Keys::Paste;
        case SDLK_FIND:
            return Keys::Find;
        case SDLK_MUTE:
            return Keys::Mute;
        case SDLK_VOLUMEUP:
            return Keys::VolumeUp;
        case SDLK_VOLUMEDOWN:
            return Keys::VolumeDown;

        case SDLK_LCTRL:
            return Keys::LeftControlKey;
        case SDLK_LSHIFT:
            return Keys::LeftShiftKey;
        case SDLK_LALT:
            return Keys::LeftAltKey;
        case SDLK_LGUI:
            return Keys::LeftMetaKey;
        case SDLK_RCTRL:
            return Keys::RightControlKey;
        case SDLK_RSHIFT:
            return Keys::RightShiftKey;
        case SDLK_RALT:
            return Keys::RightAltKey;
        case SDLK_RGUI:
            return Keys::RightMetaKey;

        case SDLK_MEDIA_PLAY_PAUSE:
            return Keys::MediaPlayPause;
        case SDLK_MEDIA_STOP:
            return Keys::MediaStopCD;
        case SDLK_MEDIA_NEXT_TRACK:
            return Keys::MediaNext;
        case SDLK_MEDIA_EJECT:
            return Keys::MediaEject;
        case SDLK_AC_HOME:
            return Keys::MediaWebsite;
        case SDLK_AC_BACK:
            return Keys::MediaBack;
        case SDLK_AC_FORWARD:
            return Keys::MediaForward;
        case SDLK_AC_STOP:
            return Keys::MediaStop;
        case SDLK_SLEEP:
            return Keys::MediaSleep;


        default:
            return Keys::None;
    }
}

}  // namespace graphics