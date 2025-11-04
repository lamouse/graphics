#pragma once
#include "imgui.h"
#include "ecs/scene/entity.hpp"
#include <span>
namespace graphics::ui {

struct MenuData {
        bool show_system_setting{};
        bool show_log{};
        bool show_out_liner{true};
        bool show_detail{true};
        bool show_status{true};
};

void draw_texture(MenuData& data, ImTextureID imguiTextureID, float aspectRatio);
void init_imgui(float scale);
// 递归绘制树节点
void ShowOutliner(std::span<ecs::Entity> instances, MenuData& data);
void show_menu(MenuData& data);
void render_status_bar(MenuData& data, float mouseX_, float mouseY_, int registry_count);
auto IsMouseControlledByImGui() -> bool;
}  // namespace graphics::ui