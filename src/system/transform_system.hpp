#pragma once
#include "ecs/components/transform_component.hpp"
#include <glm/glm.hpp>
#include "core/frame_info.hpp"
namespace graphics {
void scale(ecs::TransformComponent& transform, float scale);
void move_model(const core::FrameInfo& frameInfo, ecs::TransformComponent& transform);
}  // namespace graphics