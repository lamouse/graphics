#include "model.hpp"
#include "system/pick_system.hpp"
namespace graphics::effects {
void move_model(const core::FrameInfo& frameInfo, ecs::TransformComponent& transform) {
    auto width = static_cast<float>(frameInfo.frame_layout.width);
    auto height = static_cast<float>(frameInfo.frame_layout.height);

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
        float world_scale = distance * 0.5f;  // 经验值，可调

        // 🚚 计算世界空间位移
        glm::vec3 world_delta = (right * ndc_dx + up * ndc_dy) * world_scale;

        // 🔄 更新位置
        transform.translate(world_delta);
    }
};

void check_pick(id_t id, render::MeshId meshId, const core::FrameInfo& frameInfo,
                ecs::RenderStateComponent& render_state, ecs::TransformComponent& transform) {
    auto pick = PickingSystem::pick(id, (*frameInfo.camera), frameInfo.input_state,
                                    static_cast<float>(frameInfo.frame_layout.width),
                                    static_cast<float>(frameInfo.frame_layout.height),
                                    frameInfo.resource_manager->getMeshVertex(meshId),
                                    frameInfo.resource_manager->getMeshIndics(meshId), transform);
    if (pick) {
        render_state.mouse_select = true;
        render_state.select_id = render_state.id;
    } else {
        render_state.mouse_select = false;
    }
};
}  // namespace graphics::effects