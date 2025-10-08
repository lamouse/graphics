#pragma once
#include "render_core/fixed_pipeline_state.h"
#include "imgui.h"
namespace graphics::ui {

/**
 *
 * @param state 这里的属性值会被修改
 */
void pipeline_state(render::PipelineState& state);

void draw_result(ImTextureID imguiTextureID, float aspectRatio);
void draw_setting();
}  // namespace graphics::ui