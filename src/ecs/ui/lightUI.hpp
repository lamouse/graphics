#pragma once
#include "ecs/components/light_component.hpp"
#include "ui/ui.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
namespace ecs {
inline void DrawLightUi(LightComponent& light) {
    ImGui::ColorEdit3("color", glm::value_ptr(light.color));
    DrawFloatControl("intensity", light.intensity, 1.f, 10.f, 1.0f, 300.f);
    DrawFloatControl("radius", light.range, .01f, .1f, .01f, 300.f);
}
}  // namespace ecs