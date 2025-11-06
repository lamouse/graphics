#pragma once
#include <imgui.h>
#include "ui/ui.hpp"
#include "ecs/components/camera_component.hpp"
#include <format>
namespace ecs {
inline void DrawCameraUI(CameraComponent& cam) {
    // NOLINTNEXTLINE(hicpp-vararg)
    ImGui::Text("%s", std::format("Aspect Ratio: {:.2f}", cam.aspect()).c_str());
    auto eye = cam.eye();
    DrawVec3ColorControl("LookAt", eye, DEFAULT_EYE, 0.1F);
    cam.setEye(eye);
    auto center = cam.center();
    DrawVec3ColorControl("Center", center, DEFAULT_CENTER, 0.1F);
    cam.setCenter(center);

    auto up = cam.up();
    DrawVec3ColorControl("Up", up, DEFAULT_UP, 0.1F);
    cam.setUp(up);
    auto z_near = cam.getNear();
    DrawFloatControl("Near", z_near, 0.01F, DEFAULT_NEAR, 0.01F, 1.F);
    cam.setNear(z_near);

    auto z_far = cam.getFar();
    DrawFloatControl("Far", z_far, 0.1F, DEFAULT_FAR, 1000.0, 100000.F);
    cam.setFar(z_far);
    auto fovy = cam.getFovy();
    DrawFloatControl("FOV", fovy, 0.1F, DEFAULT_FOVY);
    cam.setFovy(fovy);
    DrawFloatControl("speed", cam.speed, 0.01f, DEFAULT_SPEED, 1.0f, 3.f);
    DrawFloatControl("sensitivity", cam.sensitivity, .001f, DEFAULT_SENSITIVITY, .001f, .1f);
}
}  // namespace ecs