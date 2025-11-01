#pragma once
#include "ecs/components/transform_component.hpp"
#include "ui/ui.hpp"
namespace ecs {
inline void DrawTransformUI(TransformComponent& tc) {
    DrawVec3ColorControl("Translation", tc.translation, glm::vec3{.0F, .0F, .0F}, .02F);
    DrawVec3ColorControl("Rotation", tc.rotation, glm::vec3{.0F, .0F, .0F}, 0.3F);
    DrawVec3ColorControl("Scale", tc.scale, glm::vec3{1.F, 1.0F, 1.F}, .0025F);
}
}  // namespace ecs