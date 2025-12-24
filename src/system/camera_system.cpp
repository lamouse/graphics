#include "system/camera_system.hpp"
#include "input/mouse.h"
#include "input/input.hpp"
namespace {

void move_forward(ecs::CameraComponent &cam, float frameSpeed) {
    glm::vec3 forward = glm::normalize(cam.center() - cam.eye());
    auto new_eye = cam.eye() + forward * frameSpeed;
    cam.setEye(new_eye);
    auto new_center = cam.center() + forward * frameSpeed;
    cam.setCenter(new_center);
}

void move_backward(ecs::CameraComponent &cam, float frameSpeed) {
    glm::vec3 forward = glm::normalize(cam.center() - cam.eye());
    auto new_eye = cam.eye() - forward * frameSpeed;
    cam.setEye(new_eye);
    auto new_center = cam.center() - forward * frameSpeed;
    cam.setCenter(new_center);
}

void move_left(ecs::CameraComponent &cam, float frameSpeed) {
    glm::vec3 forward = glm::normalize(cam.center() - cam.eye());
    glm::vec3 right = glm::normalize(glm::cross(cam.up(), forward));
    auto new_eye = cam.eye() - right * frameSpeed;
    cam.setEye(new_eye);
    auto new_center = cam.center() - right * frameSpeed;
    cam.setCenter(new_center);
}

void move_right(ecs::CameraComponent &cam, float frameSpeed) {
    glm::vec3 forward = glm::normalize(cam.center() - cam.eye());
    glm::vec3 right = glm::normalize(glm::cross(cam.up(), forward));
    auto new_eye = cam.eye() + right * frameSpeed;
    cam.setEye(new_eye);
    auto new_center = cam.center() + right * frameSpeed;
    cam.setCenter(new_center);
}

void change_fov(ecs::CameraComponent &cam, float scroll) {
    auto fov = ecs::DEFAULT_FOVY - scroll;
    if (fov <= 1.f) {
        fov = 1.f;
    }
    if (fov >= 45.f) {
        fov = 45.f;
    }
    cam.setFovy(fov);
}

void rotating(ecs::CameraComponent &cam, float mouseRelativeX_, float mouseRelativeY_) {
    glm::vec3 toEye = cam.eye() - cam.center();
    float dist = glm::length(toEye);
    if (dist < 0.1f) {
        return;
    }

    toEye = glm::normalize(toEye);

    float dx = mouseRelativeX_ * cam.sensitivity;
    float dy = -mouseRelativeY_ * cam.sensitivity;

    // 🔁 1. 先绕 WORLD_UP 旋转（yaw）—— 左右
    glm::mat4 yawRot = glm::rotate(glm::mat4(1.0f), -dx, cam.up());
    toEye = glm::vec3(yawRot * glm::vec4(toEye, 0.0f));

    // 🔁 2. 重新计算 right（因为 toEye 变了）
    glm::vec3 cam_right = glm::cross(toEye, cam.up());
    if (glm::length(cam_right) < 1e-6f) {
        cam_right = glm::vec3(1, 0, 0);  // fallback
    } else {
        cam_right = glm::normalize(cam_right);
    }

    // 🔽 3. 绕 right 旋转（pitch）—— 上下
    glm::mat4 pitchRot = glm::rotate(glm::mat4(1.0f), -dy, cam_right);
    glm::vec3 newToEye = glm::vec3(pitchRot * glm::vec4(toEye, 0.0f));

    // ✅ 限制 pitch，防止翻转
    float dot = glm::dot(newToEye, cam.up());
    if (dot > 0.99f || dot < -0.99f) {
        // 角度太大，拒绝更新
    } else {
        toEye = newToEye;
    }

    cam.setEye(cam.center() + toEye * dist);
}

}  // namespace

namespace graphics {
void CameraSystem::update(ecs::CameraComponent &cam, input::InputSystem *input, float deltaTime) {
    if (!input) {
        return;
    }

    // 计算相机坐标系
    // glm::vec3 forward = glm::normalize(cam.center() - cam.eye());
    // glm::vec3 right = glm::normalize(glm::cross(cam.up(), forward));
    // glm::vec3 up = glm::normalize(glm::cross(forward, right));

    float frameSpeed = cam.speed * deltaTime;

    // 🚶 WASD 移动
    // if (state.key == core::InputKey::W && state.key_down.Value()) {
    //     move_forward(cam, frameSpeed);
    // }
    // if (state.key == core::InputKey::S && state.key_down.Value()) {
    //     move_backward(cam, frameSpeed);
    // }

    // if (state.key == core::InputKey::A && state.key_down.Value()) {
    //     move_left(cam, frameSpeed);
    // }
    // if (state.key == core::InputKey::D && state.key_down.Value()) {
    //     move_right(cam, frameSpeed);
    // }

    // 🔁 右键旋转视角（如果你还需要旋转功能）
    auto *mouse = input->GetMouse();
    if (mouse->IsPressed(input::MouseButton::Right)) {
        rotating(cam, mouse->GetRelative().x, mouse->GetRelative().y);
    }

    // 🧮 缩放
    change_fov(cam, mouse->GetScrollOffset().y);
}
}  // namespace graphics