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

void show_ui_debug_window() {
    static bool show_window = false;
    ImGui::Checkbox("show ui debug", &show_window);
    if (show_window) {
        ImGui::ShowMetricsWindow();
    }
}

}  // namespace
namespace graphics {
void draw_setting(bool& show) {
    if (show) {
        ImGui::Begin("\ueb51 系统设置", &show);
        vsync_setting();
        log_settings();
        ImGui::End();
    }
}
}  // namespace graphics