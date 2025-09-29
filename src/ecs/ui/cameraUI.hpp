#pragma once
#include <imgui.h>
#include "ui/ui.hpp"
#include "ecs/components/camera_component.hpp"
#include <format>
namespace ecs {
inline void DrawCameraUI(CameraComponent& cam) {
    // NOLINTNEXTLINE(hicpp-vararg)
    ImGui::Text("%s", std::format("Aspect Ratio: {:.2f}", cam.extentAspectRation).c_str());
    DrawVec3ColorControl("LookAt", cam.lookAt, glm::vec3{.0F, .0F, 5.F}, 0.1F);
    DrawVec3ColorControl("Center", cam.center, glm::vec3{.0F, .0F, .0F}, 0.1F);
    DrawVec3ColorControl("Up", cam.up, glm::vec3{.0F, 1.0F, 0.F}, 0.1F);
    ImGui::DragFloat("Near", &cam.near, 0.1F);
    ImGui::DragFloat("Far", &cam.far, 0.1F);
    ImGui::DragFloat("FOV", &cam.fovy, 0.1F);
}
}