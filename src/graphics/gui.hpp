#pragma once
#include "imgui.h"
#include "ecs/component.hpp"
#include <span>
#include <string_view>
#include "common/settings.hpp"
#include "world/world.hpp"

namespace graphics::ui {

struct StatusBarData {
        float mouseX_;
        float mouseY_;
        int registry_count;
        int build_shaders;
        std::string device_name;
};

void draw_texture(settings::MenuData& data, ImTextureID imguiTextureID, float aspectRatio);
void init_imgui(float scale);
// 递归绘制树节点
void showOutliner(world::World& world, settings::MenuData& data);
void show_menu(settings::MenuData& data);
void render_status_bar(settings::MenuData& menuData, StatusBarData& barData);
auto IsMouseControlledByImGui() -> bool;
}  // namespace graphics::ui