// module;
#include "setting_ui.hpp"
#include "common/settings.hpp"
#include "ui/ui.hpp"
#include <imgui.h>
#include <format>
#include <ranges>
#include <vector>

// module setting;
namespace {

void vsync_setting() {
    auto canon = settings::enums::EnumMetadata<settings::enums::VSyncMode>::canonicalizations();
    std::vector<const char*> names;
    for (auto& key : canon | std::views::keys) {
        names.push_back(key.c_str());
    }

    static int item_current = static_cast<int>(settings::values.vsync_mode.GetValue());
    ImGui::Combo("vsync mode", &item_current, names.data(), static_cast<int>(names.size()));
    const auto vSyncMode = settings::enums::ToEnum<settings::enums::VSyncMode>(names[item_current]);
    settings::values.vsync_mode.SetValue(vSyncMode);
}

void log_settings() {
    auto canon = settings::enums::EnumMetadata<settings::enums::LogLevel>::canonicalizations();
    std::vector<const char*> names;
    for (auto& key : canon | std::views::keys) {
        names.push_back(key.c_str());
    }

    static int item_current = static_cast<int>(settings::values.log_level.GetValue());
    ImGui::Combo("log level", &item_current, names.data(), static_cast<int>(names.size()));
    const auto level = settings::enums::ToEnum<settings::enums::LogLevel>(names[item_current]);
    settings::values.log_level.SetValue(level);
    ImGui::SameLine();
}

void fps() {
    ImGuiIO const& io = ImGui::GetIO();
    (void)io;
    ImVec2 main_pos = ImGui::GetMainViewport()->Pos;
    auto main_size = ImGui::GetMainViewport()->Size;
    bool fps_open = false;
    ImGui::SetNextWindowBgAlpha(.0f);
    ImGui::SetNextWindowPos({main_pos.x + main_size.x, main_pos.y}, ImGuiCond_Always,
                            ImVec2(1.01F, 0.0F));
    ImGui::Begin("Window 1", &fps_open,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoMouseInputs | ImGuiWindowFlags_NoScrollbar |
                     ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoBackground |
                     ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::TextColored({0.0F, 1.0F, 0.0F, 1.0F}, "FPS: %.1f ", io.Framerate);
    ImGui::End();
}
void show_fps() {
    static bool show_fps = false;
    ImGui::Checkbox("show fps", &show_fps);
    if (show_fps) {
        fps();
    }
}

void show_ui_debug_window() {
    static bool show_window = false;
    ImGui::Checkbox("show ui debug", &show_window);
    if (show_window) {
        ImGui::ShowMetricsWindow();
    }
}

}  // namespace
namespace graphics {
void draw_setting() {
    ImGui::Begin("系统设置");
    show_fps();
    show_ui_debug_window();
    vsync_setting();
    log_settings();
    ImGui::End();
}
}  // namespace graphics