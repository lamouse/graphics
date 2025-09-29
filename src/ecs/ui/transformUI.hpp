#pragma once
#include <imgui.h>
#include "ecs/components/transform_component.hpp"
namespace ecs {
inline void DrawTransformUI(TransformComponent& tc) {
    ImGui::DragFloat3("Translation", &tc.translation.x, 1.F);
    ImGui::DragFloat3("Rotation", &tc.rotation.x, 1.F);
    ImGui::DragFloat3("Scale", &tc.scale.x, .0025F);
}
}  // namespace ecs