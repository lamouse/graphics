#pragma once
#include "render_core/fixed_pipeline_state.h"
#include "imgui.h"
#include "ecs/scene/entity.hpp"
#include <string>
#include <span>
namespace graphics::ui {



struct MenuData{
    bool show_system_setting;
    bool show_log;
};

/**
 *
 * @param state 这里的属性值会被修改
 */
void pipeline_state(render::PipelineState& state);

void draw_result(ImTextureID imguiTextureID, float aspectRatio);
void init_imgui(float scale);
// 递归绘制树节点
void ShowOutliner(std::span<ecs::Entity> instances);
void show_menu(MenuData& data);
}  // namespace graphics::ui