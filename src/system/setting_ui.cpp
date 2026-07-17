// module;
#include "setting_ui.hpp"
#include "common/settings.hpp"
#include "common/enum_util.hpp"
#include <imgui.h>
#include <spdlog/spdlog.h>
#include <ranges>
#include <vector>

// module setting;
namespace {
auto to_bool(const std::string& s) -> bool {
    if (s == "1" || s == "true" || s == "True" || s == "yes") {
        return true;
    }
    if (s == "0" || s == "false" || s == "False" || s == "no") {
        return false;
    }
    throw std::invalid_argument("Invalid bool string: " + s);
}

void vsync_setting() {
    const std::vector<const char*> names = common::enum_to_c_str_list<settings::enums::VSyncMode>();

    static int item_current = static_cast<int>(settings::values.vsync_mode.GetValue());
    ImGui::Combo("vsync mode", &item_current, names.data(), static_cast<int>(names.size()));
    const auto vSyncMode = common::string_to_enum<settings::enums::VSyncMode>(names[item_current]);
    settings::values.vsync_mode.SetValue(vSyncMode.value());
}

void log_settings() {
    ImGui::Separator();
    ImGui::Text("log setting");

    auto& render_category =
        settings::values.linkage.by_category.find(settings::Category::log)->second;
    for (auto* setting : render_category) {
        if (setting->TypeId() == std::type_index(typeid(settings::enums::LogLevel))) {
            auto value_string = setting->ToString();
            static int current_level = std::stoi(value_string);
            const std::vector<const char*> names = common::enum_to_c_str_list<settings::enums::LogLevel>();
            if (ImGui::Combo(setting->GetLabel().c_str(), &current_level, names.data(),
                             static_cast<int>(names.size()))) {
                setting->LoadString(std::to_string(current_level));
            }
        }

        if (setting->TypeId() == std::type_index(typeid(bool))) {
            auto value_string = setting->ToString();
            bool tmp = to_bool(value_string);
            if (!setting->RuntimeModifiable()) {
                ImGui::BeginDisabled(true);  // 禁用交互
            }
            ImGui::Checkbox(setting->GetLabel().c_str(), &tmp);
            if (!setting->RuntimeModifiable()) {
                ImGui::EndDisabled();
            } else {
                setting->LoadString(std::to_string(tmp));
            }
        }
    }
}

void render_setting() {
    ImGui::Separator();
    ImGui::Text("render setting");
    auto& render_category =
        settings::values.linkage.by_category.find(settings::Category::render)->second;
    for (auto* setting : render_category) {
        if (setting->TypeId() == std::type_index(typeid(bool))) {
            auto value_string = setting->ToString();
            bool tmp = to_bool(value_string);
            if (!setting->RuntimeModifiable()) {
                ImGui::BeginDisabled(true);  // 禁用交互
            }
            ImGui::Checkbox(setting->GetLabel().c_str(), &tmp);
            if (!setting->RuntimeModifiable()) {
                ImGui::EndDisabled();
            } else {
                dynamic_cast<settings::Setting<bool, false>*>(setting)->SetValue(tmp);
            }
        }
    }
}

}  // namespace
namespace graphics {
void draw_setting(bool& show) {
    if (show) {
        ImGui::Begin("\ueb51 系统设置", &show);
        vsync_setting();
        log_settings();
        render_setting();

        ImGui::Separator();
        if (ImGui::Button("保存设置")) {
            settings::save_settings();
        }

        ImGui::End();
    }
}
}  // namespace graphics