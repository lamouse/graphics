#include "model.hpp"

#include "system/pick_system.hpp"
namespace graphics::effects {
void move_model(const core::FrameInfo& frameInfo, ecs::TransformComponent& transform) {
    auto width = static_cast<float>(frameInfo.window_width);
    auto height = static_cast<float>(frameInfo.window_hight);

    // 📌 获取相对鼠标移动（像素）
    float dx = frameInfo.input_state.mouseRelativeX_;
    float dy = frameInfo.input_state.mouseRelativeY_;

    if (std::abs(dx) > 1e-6f || std::abs(dy) > 1e-6f) {
        // 🌐 转换：屏幕像素移动 → NDC 移动
        float ndc_dx = 2.0f * dx / width;
        float ndc_dy = 2.0f * dy / height;

        auto right = glm::vec3(frameInfo.camera->right());  // X 轴
        auto up = glm::vec3(frameInfo.camera->up());        // Y 轴

        // 📏 计算当前距离下的“NDC 单位长度”对应的世界距离
        float distance = glm::distance(transform.translation, frameInfo.camera->getPosition());
        float world_scale = distance * 0.55f;  // 经验值，可调

        // 🚚 计算世界空间位移
        glm::vec3 world_delta = (right * ndc_dx + up * ndc_dy) * world_scale;

        // 🔄 更新位置
        transform.translate(world_delta);
    }
};

void move_model(const core::FrameInfo& frameInfo, ecs::TransformComponent& transform,
                bool startDragThisFrame,
                float& out_initialWorldZ,         // 👈 只需记录一个 Z 值（更轻量）
                glm::vec3& out_dragStartWorldPos  // 仍需要完整位置用于 delta 计算
) {
    auto width = static_cast<float>(frameInfo.window_width);
    auto height = static_cast<float>(frameInfo.window_hight);

    float mouseX = frameInfo.input_state.mouseX_;
    float mouseY = frameInfo.input_state.mouseY_;

    // === 构建射线（同前）===
    float ndcX = (2.0f * mouseX / width) - 1.0f;
    float ndcY = (2.0f * mouseY / height);

    glm::vec4 rayStartNDC(ndcX, ndcY, -1.0f, 1.0f);
    glm::vec4 rayEndNDC(ndcX, ndcY, 1.0f, 1.0f);

    glm::mat4 view = frameInfo.camera->getView();
    glm::mat4 proj = frameInfo.camera->getProjection();
    glm::mat4 invVP = glm::inverse(proj * view);

    glm::vec4 rayStartWorld = invVP * rayStartNDC;
    glm::vec4 rayEndWorld = invVP * rayEndNDC;
    rayStartWorld /= rayStartWorld.w;
    rayEndWorld /= rayEndWorld.w;

    auto rayOrigin = glm::vec3(rayStartWorld);
    glm::vec3 rayDir = glm::normalize(glm::vec3(rayEndWorld - rayStartWorld));

    // === 关键：使用固定 Z 平面（世界空间）===
    float planeZ = startDragThisFrame ? transform.translation.z : out_initialWorldZ;

    // 射线与 Z = planeZ 平面相交
    // 射线: P(t) = rayOrigin + t * rayDir
    // 求 t 使得 P.z = planeZ
    if (std::abs(rayDir.z) < 1e-6f) {
        return;  // 射线平行于 XY 平面，无法相交
    }

    float t = (planeZ - rayOrigin.z) / rayDir.z;
    glm::vec3 intersection = rayOrigin + t * rayDir;

    // === 更新位置 ===
    if (startDragThisFrame) {
        out_initialWorldZ = transform.translation.z;
        out_dragStartWorldPos = intersection;
        // 注意：此时 intersection.z ≈ planeZ，但可能有浮点误差
        // 我们强制保持原始 Z
    } else {
        glm::vec3 delta = intersection - out_dragStartWorldPos;
        // 只应用 X 和 Y 的变化，Z 锁定为初始值
        transform.translation.x = transform.translation.x + delta.x;
        transform.translation.y = transform.translation.y + delta.y;
        transform.translation.z = out_initialWorldZ;  // 确保 Z 不变
    }
}

void check_pick(id_t id, render::MeshId meshId, const core::FrameInfo& frameInfo,
                ecs::RenderStateComponent& render_state, ecs::TransformComponent& transform) {
    auto pick = PickingSystem::pick(id, (*frameInfo.camera), frameInfo.input_state,
                                    static_cast<float>(frameInfo.window_width),
                                    static_cast<float>(frameInfo.window_hight),
                                    frameInfo.resource_manager->getMeshVertex(meshId),
                                    frameInfo.resource_manager->getMeshIndics(meshId), transform);
    if (pick) {
        render_state.mouse_select = true;
        render_state.select_id = render_state.id;
    } else {
        render_state.mouse_select = false;
    }
};

auto uploadMeshMaterialResource(graphics::ResourceManager& manager, const SubMesh& subMesh)
    -> std::tuple<MeshMaterialResource, MaterialUBO> {
    MeshMaterialResource materialResource{};
    MaterialUBO materialUBO{};
    materialUBO.ambient = {subMesh.material.ambientColor, 1.f};
    materialUBO.diffuse = {subMesh.material.diffuseColor, 1.f};
    materialUBO.specular = subMesh.material.specularColor;
    materialUBO.shininess = subMesh.material.shininess;
    if (!subMesh.material.ambientTextures.empty()) {
        materialResource.ambientTextures =
            manager.addTexture("./images/" + subMesh.material.ambientTextures[0]);
    }else {
        materialResource.ambientTextures =
            manager.getTexture(std::string(DEFAULT_1X1_WRITE_TEXTURE));
    }

    if (!subMesh.material.diffuseTextures.empty()) {
        materialResource.diffuseTextures =
            manager.addTexture("./images/" + subMesh.material.diffuseTextures[0]);
    }else {
        materialResource.diffuseTextures =
            manager.getTexture(std::string(DEFAULT_1X1_WRITE_TEXTURE));
    }

    if (!subMesh.material.specularTextures.empty()) {
        materialResource.specularTextures =
            manager.addTexture("./images/" + subMesh.material.specularTextures[0]);
    } else {
        materialResource.specularTextures =
            manager.getTexture(std::string(DEFAULT_1X1_WRITE_TEXTURE));
    }
    if (!subMesh.material.normalTextures.empty()) {
        materialResource.normalTextures =
            manager.addTexture("./images/" + subMesh.material.normalTextures[0]);
    } else {
        materialResource.normalTextures =
            manager.getTexture(std::string(DEFAULT_1X1_WRITE_TEXTURE));
    }
    return {materialResource, materialUBO};
}

}  // namespace graphics::effects