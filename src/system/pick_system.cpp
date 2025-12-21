// picking_system.cpp
#include "pick_system.hpp"
#include <glm/gtx/norm.hpp>
#include "system/embree_picker.hpp"
#include <tracy/Tracy.hpp>

auto get_embree_picker() -> graphics::EmbreePicker* {
    static graphics::EmbreePicker packer;
    return &packer;
}

namespace graphics {

void PickingSystem::upload_vertex(id_t id, id_t mesh, std::span<const glm::vec3> localVertices,
                                  std::span<const uint32_t> indices) {
    auto* picker = get_embree_picker();
    picker->buildMesh(id, mesh, localVertices, indices);
}

void PickingSystem::update_transform(id_t id, const ecs::TransformComponent& transform) {
    auto* picker = get_embree_picker();
    picker->updateTransform(id, transform);
}

void PickingSystem::commit() {
    ZoneScopedN("PickingSystem::commit");
    auto* picker = get_embree_picker();
    picker->commit();
    picker->warmUp();
}

auto PickingSystem::pick(const core::Camera& camera, float mouseX, float mouseY, float windowWidth,
                         float windowHeight) -> std::optional<PickResult> {
    // 📌 1. 获取视图-投影矩阵
    glm::mat4 viewProj = camera.getProjection() * camera.getView();
    glm::mat4 invViewProj = glm::inverse(viewProj);

    // 📌 2. 屏幕坐标 -> NDC [-1, 1]
    float ndcX = (2.0f * mouseX) / windowWidth - 1.0f;
    float ndcY = (2.0f * mouseY) / windowHeight - 1.0f;  //
    float ndcZ = -1.0f;                                  // 使用 NDC 近平面 (z = -1)

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
    picker->commit();
    return picker->pick(rayOrigin, rayDirection);
}

}  // namespace graphics