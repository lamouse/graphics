#pragma once
#include "imgui.h"
#include "ecs/scene/entity.hpp"
#include <span>
#include <string_view>

namespace graphics::ui {

struct MenuData {
        bool show_system_setting{};
        bool show_log{};
        bool show_out_liner{true};
        bool show_detail{true};
        bool show_status{true};
};

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

void draw_texture(MenuData& data, ImTextureID imguiTextureID, float aspectRatio);
void init_imgui(float scale);
// 递归绘制树节点
void ShowOutliner(std::span<Outliner> outlineres, MenuData& data);
void show_menu(MenuData& data);
void render_status_bar(MenuData& menuData, StatusBarData& barData);
auto IsMouseControlledByImGui() -> bool;
}  // namespace graphics::ui