#pragma once
#include "ecs/components/transform_component.hpp"
#include "ui/ui.hpp"
namespace ecs {
inline void DrawTransformUI(TransformComponent& tc) {
    DrawVec3ColorControl("Translation", tc.translation, glm::vec3{.0F, .0F, .0F}, .02F);
    DrawVec3ColorControl("Rotation", tc.rotation, glm::vec3{.0F, .0F, .0F}, 0.3F);
    DrawFloatControl("Scale", tc.scale.x, .01f, 1.f, 0.01f, 10.f);
    tc.scale.y = tc.scale.x;
    tc.scale.z = tc.scale.x;
}
}  // namespace ecs