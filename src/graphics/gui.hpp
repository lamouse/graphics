#pragma once
#include "imgui.h"
#include "common/settings.hpp"
#include "world/world.hpp"
#include "effects//effect.hpp"

namespace graphics::ui {

struct StatusBarData {
        float mouseX_{};
        float mouseY_{};
        int registry_count{};
        int build_shaders{};
        std::string device_name;
};

using AddModelInfo = std::variant<bool, graphics::effects::ModelEffectInfo>;

void draw_texture(settings::MenuData& data, ImTextureID imguiTextureID, float aspectRatio);
// 递归绘制树节点
void showOutliner(world::World& world, ResourceManager& resourceManager, settings::MenuData& data);
void show_menu(settings::MenuData& data);
void render_status_bar(settings::MenuData& menuData, StatusBarData& barData);
auto IsMouseControlledByImGui() -> bool;
auto IsKeyboardControlledByImGui() -> bool;
void add_imgui_event(const std::function<void()>& event_func);
void run_all_imgui_event();
auto add_model(std::string_view model_path) -> AddModelInfo;
}  // namespace graphics::ui