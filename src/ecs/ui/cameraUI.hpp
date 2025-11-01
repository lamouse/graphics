#pragma once
#include <imgui.h>
#include "ui/ui.hpp"
#include "ecs/components/camera_component.hpp"
#include <format>
namespace ecs {
inline void DrawCameraUI(CameraComponent& cam) {
    // NOLINTNEXTLINE(hicpp-vararg)
    ImGui::Text("%s", std::format("Aspect Ratio: {:.2f}", cam.extentAspectRation).c_str());
    DrawVec3ColorControl("LookAt", cam.eye, DEFAULT_EYE, 0.1F);
    DrawVec3ColorControl("Center", cam.center, DEFAULT_CENTER, 0.1F);
    DrawVec3ColorControl("Up", cam.up, DEFAULT_UP, 0.1F);
    DrawFloatControl("Near", cam.z_near, 0.01F, DEFAULT_NEAR, 0.01F, 1.F);
    DrawFloatControl("Far", cam.z_far, 0.1F, DEFAULT_FAR, 1000.0, 100000.F);
    DrawFloatControl("FOV", cam.fovy, 0.1F, DEFAULT_FOVY);
}
}  // namespace ecs