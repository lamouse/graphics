// picking_system.hpp
#pragma once
#include "core/camera/camera.hpp"
#include "core/input.hpp"
#include "ecs/components/transform_component.hpp"
#include <glm/glm.hpp>
#include <optional>
#include <span>
#include "resource/id.hpp"

namespace graphics {

struct PickResult {
        glm::vec3 position;
        float distance;
        uint32_t primitiveId;  // 三角形 ID
        id_t id;
};

class PickingSystem {
    public:
        static void upload_vertex(id_t id, std::span<const glm::vec3> localVertices,
                                  std::span<const uint32_t> indices);
        static void update_transform(id_t id, const ecs::TransformComponent& transform);

        static auto pick(const core::Camera& camera, float mouseX, float mouseY, float windowWidth,
                  float windowHeight) -> std::optional<PickResult>;

        // 主要拾取函数
        static auto pick(id_t id, const core::Camera& camera, const core::InputState& key_state,
                         float windowWidth, float windowHeight,
                         std::span<const glm::vec3> localVertices,
                         std::span<const uint32_t> indices,
                         const ecs::TransformComponent& transform) -> std::optional<PickResult>;
};

}  // namespace graphics