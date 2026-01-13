#pragma once
#include "ecs/components/light_component.hpp"
#include "ui/ui.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include "imGuIZMOquat.h"
namespace ecs {
inline void DrawLightUi(LightComponent& light) {
    if (light.type == LightType::Directional) {
        ImGui::Text("Directional Light");
    } else if (light.type == LightType::Point) {
        ImGui::Text("Point Light");
    } else if (light.type == LightType::Spot) {
        ImGui::Text("Spot Light");
    }
    ImGui::ColorEdit3("color", glm::value_ptr(light.color));

    if (light.type == LightType::Point) {
        DrawFloatControl("intensity", light.intensity, 1.f, 10.f, 1.0f, 300.f);
        DrawFloatControl("radius", light.range, .01f, .1f, .01f, 300.f);
    }
    if (light.type == LightType::Directional) {
        ImGui::DragFloat3("direction", glm::value_ptr(light.direction), 0.1f, -1.f, 1.f);

        vec3 dir = vec3(-light.direction.x, -light.direction.y, light.direction.z);
        ImGui::gizmo3D("##Light Direction", dir, 120.f,  imguiGizmo::modeDirection);
        light.direction = glm::vec3(-dir.x, -dir.y, dir.z);

        DrawFloatControl("intensity", light.intensity, .01f, .5f, .0f, 1.f);


    }
}
}  // namespace ecs