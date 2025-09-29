#pragma once
#include <imgui.h>
#include "ui/ui.hpp"
#include "ecs/components/camera_component.hpp"
#include <format>
namespace ecs {
inline void DrawCameraUI(CameraComponent& cam) {
    // NOLINTNEXTLINE(hicpp-vararg)
    ImGui::Text("%s", std::format("Aspect Ratio: {:.2f}", cam.extentAspectRation).c_str());
    DrawVec3ColorControl("LookAt", cam.lookAt, DEFAULT_LOOK_AT, 0.1F);
    DrawVec3ColorControl("Center", cam.center, DEFAULT_CENTER, 0.1F);
    DrawVec3ColorControl("Up", cam.up, DEFAULT_UP, 0.1F);

    DrawFloatControl("Near",cam.near, 0.1F, DEFAULT_NEAR);
    DrawFloatControl("Far",cam.near, 0.1F, DEFAULT_FAR);
    DrawFloatControl("FOV",cam.near, 0.1F, DEFAULT_FOVY);
}
}