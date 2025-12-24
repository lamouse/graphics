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
}  // namespace

namespace imgui::qt {
void mouse_press_event(QMouseEvent* event) {
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
    ImGuiIO& io = ImGui::GetIO();
    io.AddMousePosEvent(static_cast<float>(event->position().x()),
                        static_cast<float>(event->position().y()));
}
void mouse_wheel_event(QWheelEvent* event) {
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
    ImGuiIO& io = ImGui::GetIO();
    io.AddFocusEvent(true);
}
void mouse_focus_out_event() {
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

}  // namespace imgui::qt
