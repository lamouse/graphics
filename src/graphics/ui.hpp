#pragma once
#include "render_core/uniforms.hpp"
#include "render_core/fixed_pipeline_state.h"
#include "imgui.h"
namespace graphics::ui {
struct ImguiDebugInfo {
        float speed;
        float look_x;
        float look_y;
        float look_z;

        float center_x;
        float center_y;
        float center_z;

        float up_x;
        float up_y;
        float up_z;

        float rotate_x;
        float rotate_y;
        float rotate_z;
        float radians;

        float z_far;
        float z_near;
};
auto init_debug_info() -> ImguiDebugInfo;
void uniform_ui(ImguiDebugInfo& debugInfo);
auto get_uniform_buffer(ImguiDebugInfo& debugInfo, float extentAspectRation)
    -> render::UniformBufferObject;

/**
 *
 * @param state 这里的属性值会被修改
 */
void pipeline_state(render::PipelineState& state);

void draw_result(ImTextureID imguiTextureID, float aspectRatio);
void draw_setting();
void draw_docked_window();
void main_ui();
void begin();
void end();
}  // namespace graphics::ui