#pragma once
#include "imgui.h"
#include "ecs/scene/entity.hpp"
#include <span>
#include <string_view>
#include "common/settings.hpp"

namespace graphics::ui {

struct StatusBarData {
        float mouseX_;
        float mouseY_;
        int registry_count;
        int build_shaders;
        std::string_view device_name;
};

struct Outliner {
        ecs::Entity entity;
        std::vector<ecs::Entity> children;
};

void draw_texture(settings::MenuData& data, ImTextureID imguiTextureID, float aspectRatio);
void init_imgui(float scale);
// 递归绘制树节点
void ShowOutliner(std::span<Outliner> outlineres, settings::MenuData& data);
void show_menu(settings::MenuData& data);
void render_status_bar(settings::MenuData& menuData, StatusBarData& barData);
auto IsMouseControlledByImGui() -> bool;
}  // namespace graphics::ui