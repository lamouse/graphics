#include "transform_system.hpp"
namespace graphics {
void scale(ecs::TransformComponent& transform, float scale) {
    constexpr float SCALE_SIZE = 0.02f;
    constexpr float MAX_SCALE_SIZE = 2.f;
    constexpr float MIN_SCALE_SIZE = 0.01f;
    auto real_scale = transform.scale.x;
    real_scale += SCALE_SIZE * scale;
    if (real_scale <= MIN_SCALE_SIZE) {
        real_scale = MIN_SCALE_SIZE;
    }
    if (real_scale > MAX_SCALE_SIZE) {
        real_scale = MAX_SCALE_SIZE;
    }
    transform.scale = glm::vec3(real_scale);
}

void move_model(const core::FrameInfo& frameInfo, ecs::TransformComponent& transform) {
    // auto width = static_cast<float>(frameInfo.window_width);
    // auto height = static_cast<float>(frameInfo.window_hight);

    // float dx = frameInfo.input_event->mouseRelativeX_;
    // float dy = frameInfo.input_event->mouseRelativeY_;

    // if (std::abs(dx) > 1e-6f || std::abs(dy) > 1e-6f) {
    //     // 当前鼠标位置 (屏幕坐标)
    //     auto mouseX = static_cast<float>(frameInfo.input_event->mouseX_);
    //     auto mouseY = static_cast<float>(frameInfo.input_event->mouseY_);

    //     // 新鼠标位置
    //     float newMouseX = mouseX + dx;
    //     float newMouseY = mouseY + dy;

    //     // 将屏幕坐标转换到 NDC [-1,1]
    //     auto toNDC = [&](float x, float y) {
    //         return glm::vec2((x / width) * 2.0f - 1.0f, (y / height) * 2.0f);
    //     };

    //     glm::vec2 ndc0 = toNDC(mouseX, mouseY);
    //     glm::vec2 ndc1 = toNDC(newMouseX, newMouseY);

    //     // 相机矩阵
    //     glm::mat4 invVP =
    //         glm::inverse(frameInfo.camera->getProjection() * frameInfo.camera->getView());

    //     auto ndcToWorld = [&](glm::vec2 ndc) {
    //         glm::vec4 clip(ndc.x, ndc.y, 1.0f, 1.0f);  // 在远平面上取点
    //         glm::vec4 world = invVP * clip;
    //         return glm::vec3(world) / world.w;
    //     };

    //     // 得到两条射线的方向
    //     glm::vec3 camPos = frameInfo.camera->getPosition();
    //     glm::vec3 ray0 = glm::normalize(ndcToWorld(ndc0) - camPos);
    //     glm::vec3 ray1 = glm::normalize(ndcToWorld(ndc1) - camPos);

    //     // 选择一个参考平面：以模型当前位置为中心，法线为相机前向
    //     glm::vec3 planePoint = transform.translation;
    //     glm::vec3 planeNormal = glm::normalize(frameInfo.camera->front());

    //     auto intersectRayPlane = [&](glm::vec3 rayDir) {
    //         float denom = glm::dot(planeNormal, rayDir);
    //         if (std::abs(denom) < 1e-6f) {
    //             return transform.translation;  // 平行情况
    //         }
    //         float t = glm::dot(planePoint - camPos, planeNormal) / denom;
    //         return camPos + rayDir * t;
    //     };

    //     glm::vec3 hit0 = intersectRayPlane(ray0);
    //     glm::vec3 hit1 = intersectRayPlane(ray1);

    //     // 世界空间位移
    //     glm::vec3 world_delta = hit1 - hit0;
    //     transform.translate(world_delta);
    // }
}
}  // namespace graphics