#include "graphics/imgui_qt.hpp"
#include <QMouseEvent>
#include <QWheelEvent>
#include <imgui.h>

namespace {
auto qt_button_to_imgui(Qt::MouseButton button) -> int {
    int mouse_button = -1;
    switch (button) {
        case Qt::LeftButton:
            mouse_button = 0;
            break;
        case Qt::RightButton:
            mouse_button = 1;
            break;
        case Qt::MiddleButton:
            mouse_button = 2;
            break;
        case Qt::BackButton:
            mouse_button = 3;
            break;
        case Qt::ForwardButton:
            mouse_button = 4;
            break;
        default:
            break;
    }
    return mouse_button;
}

void UpdateKeyModifiers(Qt::KeyboardModifiers qt_modifiers) {
    ImGuiIO& io = ImGui::GetIO();
    io.AddKeyEvent(ImGuiMod_Ctrl, (qt_modifiers & Qt::KeyboardModifier::ControlModifier) != 0);
    io.AddKeyEvent(ImGuiMod_Shift, (qt_modifiers & Qt::KeyboardModifier::ShiftModifier) != 0);
    io.AddKeyEvent(ImGuiMod_Alt, (qt_modifiers & Qt::KeyboardModifier::AltModifier) != 0);
    io.AddKeyEvent(ImGuiMod_Super, (qt_modifiers & Qt::KeyboardModifier::MetaModifier) != 0);
}

#include <QKeyEvent>
#include "imgui.h"

auto Qt_KeyToImGuiKey(Qt::Key qt_key) -> ImGuiKey {
    // 小键盘（Keypad）键：Qt 有独立的 Key_KeypadX
    switch (qt_key) {
        case Qt::Key_0:
            return ImGuiKey_0;
        case Qt::Key_1:
            return ImGuiKey_1;
        case Qt::Key_2:
            return ImGuiKey_2;
        case Qt::Key_3:
            return ImGuiKey_3;
        case Qt::Key_4:
            return ImGuiKey_4;
        case Qt::Key_5:
            return ImGuiKey_5;
        case Qt::Key_6:
            return ImGuiKey_6;
        case Qt::Key_7:
            return ImGuiKey_7;
        case Qt::Key_8:
            return ImGuiKey_8;
        case Qt::Key_9:
            return ImGuiKey_9;

        case Qt::Key_A:
            return ImGuiKey_A;
        case Qt::Key_B:
            return ImGuiKey_B;
        case Qt::Key_C:
            return ImGuiKey_C;
        case Qt::Key_D:
            return ImGuiKey_D;
        case Qt::Key_E:
            return ImGuiKey_E;
        case Qt::Key_F:
            return ImGuiKey_F;
        case Qt::Key_G:
            return ImGuiKey_G;
        case Qt::Key_H:
            return ImGuiKey_H;
        case Qt::Key_I:
            return ImGuiKey_I;
        case Qt::Key_J:
            return ImGuiKey_J;
        case Qt::Key_K:
            return ImGuiKey_K;
        case Qt::Key_L:
            return ImGuiKey_L;
        case Qt::Key_M:
            return ImGuiKey_M;
        case Qt::Key_N:
            return ImGuiKey_N;
        case Qt::Key_O:
            return ImGuiKey_O;
        case Qt::Key_P:
            return ImGuiKey_P;
        case Qt::Key_Q:
            return ImGuiKey_Q;
        case Qt::Key_R:
            return ImGuiKey_R;
        case Qt::Key_S:
            return ImGuiKey_S;
        case Qt::Key_T:
            return ImGuiKey_T;
        case Qt::Key_U:
            return ImGuiKey_U;
        case Qt::Key_V:
            return ImGuiKey_V;
        case Qt::Key_W:
            return ImGuiKey_W;
        case Qt::Key_X:
            return ImGuiKey_X;
        case Qt::Key_Y:
            return ImGuiKey_Y;
        case Qt::Key_Z:
            return ImGuiKey_Z;

        case Qt::Key_F1:
            return ImGuiKey_F1;
        case Qt::Key_F2:
            return ImGuiKey_F2;
        case Qt::Key_F3:
            return ImGuiKey_F3;
        case Qt::Key_F4:
            return ImGuiKey_F4;
        case Qt::Key_F5:
            return ImGuiKey_F5;
        case Qt::Key_F6:
            return ImGuiKey_F6;
        case Qt::Key_F7:
            return ImGuiKey_F7;
        case Qt::Key_F8:
            return ImGuiKey_F8;
        case Qt::Key_F9:
            return ImGuiKey_F9;
        case Qt::Key_F10:
            return ImGuiKey_F10;
        case Qt::Key_F11:
            return ImGuiKey_F11;
        case Qt::Key_F12:
            return ImGuiKey_F12;
        case Qt::Key_F13:
            return ImGuiKey_F13;
        case Qt::Key_F14:
            return ImGuiKey_F14;
        case Qt::Key_F15:
            return ImGuiKey_F15;
        case Qt::Key_F16:
            return ImGuiKey_F16;
        case Qt::Key_F17:
            return ImGuiKey_F17;
        case Qt::Key_F18:
            return ImGuiKey_F18;
        case Qt::Key_F19:
            return ImGuiKey_F19;
        case Qt::Key_F20:
            return ImGuiKey_F20;
        case Qt::Key_F21:
            return ImGuiKey_F21;
        case Qt::Key_F22:
            return ImGuiKey_F22;
        case Qt::Key_F23:
            return ImGuiKey_F23;
        case Qt::Key_F24:
            return ImGuiKey_F24;

        case Qt::Key_Tab:
            return ImGuiKey_Tab;
        case Qt::Key_Left:
            return ImGuiKey_LeftArrow;
        case Qt::Key_Right:
            return ImGuiKey_RightArrow;
        case Qt::Key_Up:
            return ImGuiKey_UpArrow;
        case Qt::Key_Down:
            return ImGuiKey_DownArrow;
        case Qt::Key_PageUp:
            return ImGuiKey_PageUp;
        case Qt::Key_PageDown:
            return ImGuiKey_PageDown;
        case Qt::Key_Home:
            return ImGuiKey_Home;
        case Qt::Key_End:
            return ImGuiKey_End;
        case Qt::Key_Insert:
            return ImGuiKey_Insert;
        case Qt::Key_Delete:
            return ImGuiKey_Delete;
        case Qt::Key_Backspace:
            return ImGuiKey_Backspace;
        case Qt::Key_Space:
            return ImGuiKey_Space;
        case Qt::Key_Return:
        case Qt::Key_Enter:
            return ImGuiKey_Enter;
        case Qt::Key_Escape:
            return ImGuiKey_Escape;
        case Qt::Key_CapsLock:
            return ImGuiKey_CapsLock;
        case Qt::Key_ScrollLock:
            return ImGuiKey_ScrollLock;
        case Qt::Key_NumLock:
            return ImGuiKey_NumLock;
        case Qt::Key_Print:
            return ImGuiKey_PrintScreen;
        case Qt::Key_Pause:
            return ImGuiKey_Pause;

        case Qt::Key_Control:
            return ImGuiKey_LeftCtrl;
        case Qt::Key_Meta:
            return ImGuiKey_LeftSuper;
        case Qt::Key_Alt:
            return ImGuiKey_LeftAlt;
        case Qt::Key_Shift:
            return ImGuiKey_LeftShift;
            // These are generic; better to use specific left/right if possible
            // But we'll map them as left by default if no better info
            break;

        // case Qt::Key_LeftControl:  return ImGuiKey_LeftCtrl;
        // case Qt::Key_RightControl: return ImGuiKey_RightCtrl;
        // case Qt::Key_LeftShift:    return ImGuiKey_LeftShift;
        // case Qt::Key_RightShift:   return ImGuiKey_RightShift;
        // case Qt::Key_LeftAlt:      return ImGuiKey_LeftAlt;
        // case Qt::Key_RightAlt:     return ImGuiKey_RightAlt;
        // case Qt::Key_LeftMeta:     return ImGuiKey_LeftSuper;
        // case Qt::Key_RightMeta:    return ImGuiKey_RightSuper;
        case Qt::Key_Menu:
            return ImGuiKey_Menu;

        // Keypad keys (Qt has explicit keypad enums)
        // case Qt::Keypad:        return ImGuiKey_Keypad0;
        // case Qt::Key_Keypad1:        return ImGuiKey_Keypad1;
        // case Qt::Key_Keypad2:        return ImGuiKey_Keypad2;
        // case Qt::Key_Keypad3:        return ImGuiKey_Keypad3;
        // case Qt::Key_Keypad4:        return ImGuiKey_Keypad4;
        // case Qt::Key_Keypad5:        return ImGuiKey_Keypad5;
        // case Qt::Key_Keypad6:        return ImGuiKey_Keypad6;
        // case Qt::Key_Keypad7:        return ImGuiKey_Keypad7;
        // case Qt::Key_Keypad8:        return ImGuiKey_Keypad8;
        // case Qt::Key_Keypad9:        return ImGuiKey_Keypad9;
        // case Qt::Key_KeypadDecimal:  return ImGuiKey_KeypadDecimal;
        // case Qt::Key_KeypadDivide:   return ImGuiKey_KeypadDivide;
        // case Qt::Key_KeypadMultiply: return ImGuiKey_KeypadMultiply;
        // case Qt::Key_KeypadSubtract: return ImGuiKey_KeypadSubtract;
        // case Qt::Key_KeypadAdd:      return ImGuiKey_KeypadAdd;
        // case Qt::Key_KeypadEnter:    return ImGuiKey_KeypadEnter;
        // case Qt::Key_KeypadEqual:    return ImGuiKey_KeypadEqual;

        // Punctuation and symbols — note: these may vary by keyboard layout!
        case Qt::Key_Comma:
            return ImGuiKey_Comma;
        case Qt::Key_Period:
            return ImGuiKey_Period;
        case Qt::Key_Semicolon:
            return ImGuiKey_Semicolon;
        case Qt::Key_Apostrophe:
            return ImGuiKey_Apostrophe;
        case Qt::Key_QuoteDbl:  // fallback if needed, but usually not used in imgui
        case Qt::Key_Minus:
            return ImGuiKey_Minus;
        case Qt::Key_Equal:
            return ImGuiKey_Equal;
        case Qt::Key_BracketLeft:
            return ImGuiKey_LeftBracket;
        case Qt::Key_BracketRight:
            return ImGuiKey_RightBracket;
        case Qt::Key_Backslash:
            return ImGuiKey_Backslash;
        case Qt::Key_NumberSign:  // '#' — not in ImGui standard keys
        case Qt::Key_Slash:
            return ImGuiKey_Slash;
        case Qt::Key_Backtab:  // Shift+Tab — usually handled separately
        case Qt::Key_QuoteLeft:
            return ImGuiKey_GraveAccent;  // often '`' or '~'

        // Application navigation keys
        case Qt::Key_Back:
            return ImGuiKey_AppBack;
        case Qt::Key_Forward:
            return ImGuiKey_AppForward;

        default:
            break;
    }

    // If not mapped, return None
    return ImGuiKey_None;
}

}  // namespace

namespace imgui::qt {
void mouse_press_event(QMouseEvent* event) {
    if (ImGui::GetCurrentContext() == nullptr) {
        return;
    }
    auto imgui_button = qt_button_to_imgui(event->button());
    if (imgui_button < 0) {
        return;
    }
    ImGuiIO& io = ImGui::GetIO();
    io.MousePos = ImVec2(static_cast<float>(event->position().x()),
                         static_cast<float>(event->position().y()));
    auto source = event->source();
    if (source == Qt::MouseEventNotSynthesized) {
        io.AddMouseSourceEvent(ImGuiMouseSource_Mouse);
    } else if (source == Qt::MouseEventSynthesizedBySystem) {
        io.AddMouseSourceEvent(ImGuiMouseSource_TouchScreen);
    }
    io.AddMouseButtonEvent(imgui_button, true);
}
void mouse_release_event(QMouseEvent* event) {
    if (ImGui::GetCurrentContext() == nullptr) {
        return;
    }
    auto imgui_button = qt_button_to_imgui(event->button());
    if (imgui_button < 0) {
        return;
    }
    ImGuiIO& io = ImGui::GetIO();
    io.MousePos = ImVec2(static_cast<float>(event->position().x()),
                         static_cast<float>(event->position().y()));
    auto source = event->source();
    if (source == Qt::MouseEventNotSynthesized) {
        io.AddMouseSourceEvent(ImGuiMouseSource_Mouse);
    } else if (source == Qt::MouseEventSynthesizedBySystem) {
        io.AddMouseSourceEvent(ImGuiMouseSource_TouchScreen);
    }
    io.AddMouseButtonEvent(imgui_button, false);
}

void mouse_move_event(QMouseEvent* event) {
    if (ImGui::GetCurrentContext() == nullptr) {
        return;
    }
    ImGuiIO& io = ImGui::GetIO();
    io.AddMousePosEvent(static_cast<float>(event->position().x()),
                        static_cast<float>(event->position().y()));
}
void mouse_wheel_event(QWheelEvent* event) {
    if (ImGui::GetCurrentContext() == nullptr) {
        return;
    }
    ImGuiIO& io = ImGui::GetIO();
    io.MouseWheel += event->angleDelta().y() / 120.0f;   // 垂直滚轮
    io.MouseWheelH += event->angleDelta().x() / 120.0f;  // 水平滚轮
    auto source = event->source();
    if (source == Qt::MouseEventNotSynthesized) {
        io.AddMouseSourceEvent(ImGuiMouseSource_Mouse);
    } else if (source == Qt::MouseEventSynthesizedBySystem) {
        io.AddMouseSourceEvent(ImGuiMouseSource_TouchScreen);
    }
}
void mouse_focus_in_event() {
    if (ImGui::GetCurrentContext() == nullptr) {
        return;
    }
    ImGuiIO& io = ImGui::GetIO();
    io.AddFocusEvent(true);
}
void mouse_focus_out_event() {
    if (ImGui::GetCurrentContext() == nullptr) {
        return;
    }
    ImGuiIO& io = ImGui::GetIO();
    io.AddFocusEvent(false);
}

void new_frame(float weight, float height) {
    if (ImGui::GetCurrentContext() == nullptr) {
        return;
    }
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(weight, height);
}

void key_press(QKeyEvent* event) {
    if (ImGui::GetCurrentContext() == nullptr) {
        return;
    }
    ImGuiIO& io = ImGui::GetIO();
    UpdateKeyModifiers(event->modifiers());
    ImGuiKey key = Qt_KeyToImGuiKey(Qt::Key(event->key()));
    io.AddKeyEvent(key, true);
}
void key_release(QKeyEvent* event) {
    if (ImGui::GetCurrentContext() == nullptr) {
        return;
    }
    ImGuiIO& io = ImGui::GetIO();
    UpdateKeyModifiers(event->modifiers());
    ImGuiKey key = Qt_KeyToImGuiKey(Qt::Key(event->key()));
    io.AddKeyEvent(key, false);
}

}  // namespace imgui::qt
