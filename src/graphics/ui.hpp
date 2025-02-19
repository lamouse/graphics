#pragma once
#include "render_core/uniforms.hpp"

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
auto get_uniform_buffer(ImguiDebugInfo& debugInfo, float extentAspectRation) -> render::UniformBufferObject;

struct UIConfig{
};

class UI{

    private:
        UIConfig config;
};

void main_ui();
void begin();
void end();
}