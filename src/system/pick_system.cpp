// picking_system.cpp
#include "pick_system.hpp"
#include <glm/gtx/norm.hpp>

#include "system/embree_picker.hpp"

auto get_embree_picker() -> graphics::EmbreePicker* {
    static graphics::EmbreePicker packer;
    return &packer;
}

namespace graphics {

auto PickingSystem::pick(id_t id, const core::Camera& camera, const core::InputState& key_state,
                         float windowWidth, float windowHeight,
                         std::span<const glm::vec3> localVertices,
                         std::span<const uint32_t> indices,
                         const ecs::TransformComponent& transform) -> std::optional<PickResult> {
    // 📌 1. 获取视图-投影矩阵
    glm::mat4 viewProj = camera.getProjection() * camera.getView();
    glm::mat4 invViewProj = glm::inverse(viewProj);

    // 📌 2. 屏幕坐标 -> NDC [-1, 1]
    float ndcX = (2.0f * key_state.mouseX_) / windowWidth - 1.0f;
    float ndcY = (2.0f * key_state.mouseY_) / windowHeight - 1.0f;  //
    float ndcZ = -1.0f;                                             // 使用 NDC 近平面 (z = -1)

    // 📌 3. 构建 NDC 射线并转换到世界空间
    glm::vec4 rayNDCStart(ndcX, ndcY, ndcZ, 1.0f);
    glm::vec4 rayNDCEnd(ndcX, ndcY, 1.0f, 1.0f);  // 指向远平面

    glm::vec4 rayWorldStart = invViewProj * rayNDCStart;
    glm::vec4 rayWorldEnd = invViewProj * rayNDCEnd;

    // 透视除法
    rayWorldStart /= rayWorldStart.w;
    rayWorldEnd /= rayWorldEnd.w;

    auto rayOrigin = glm::vec3(rayWorldStart);
    glm::vec3 rayDir = glm::vec3(rayWorldEnd) - glm::vec3(rayWorldStart);

    // ✅ 防止零向量
    if (glm::length2(rayDir) < 1e-6f) {
        return std::nullopt;
    }
    glm::vec3 rayDirection = glm::normalize(rayDir);
    auto* picker = get_embree_picker();

    picker->buildMesh(id, localVertices, indices);
    picker->updateTransform(id, transform);
    picker->commit();
    return picker->pick(id, rayOrigin, rayDirection);
}

}  // namespace graphics