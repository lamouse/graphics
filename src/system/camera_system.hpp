#pragma once
#include "ecs/components/camera_component.hpp"
#include "core/input.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace graphics {

struct CameraSystem {
    public:
        static void update(ecs::CameraComponent& cam, core::InputState state,
                           float deltaTime = 1.0f / 60.0f) {
            float moveSpeed = 2.0f;

            // 计算相机坐标系
            glm::vec3 forward = glm::normalize(cam.center() - cam.eye());
            glm::vec3 right = glm::normalize(glm::cross(cam.up(), forward));
            glm::vec3 up = glm::normalize(glm::cross(forward, right));

            float frameSpeed = moveSpeed * deltaTime;

            // 🚶 WASD 移动
            if (state.key == core::InputKey::W && state.key_down.Value()) {
                auto new_eye = cam.eye() + forward * frameSpeed;
                cam.setEye(new_eye);
                auto new_center = cam.center() + forward * frameSpeed;
                cam.setCenter(new_center);
            }
            if (state.key == core::InputKey::S && state.key_down.Value()) {
                auto new_eye = cam.eye() - forward * frameSpeed;
                cam.setEye(new_eye);
                auto new_center = cam.center() - forward * frameSpeed;
                cam.setCenter(new_center);
            }
            if (state.key == core::InputKey::A && state.key_down.Value()) {
                auto new_eye = cam.eye() - right * frameSpeed;
                cam.setEye(new_eye);
                auto new_center = cam.center() - right * frameSpeed;
                cam.setCenter(new_center);
            }
            if (state.key == core::InputKey::D && state.key_down.Value()) {
                auto new_eye = cam.eye() + right * frameSpeed;
                cam.setEye(new_eye);
                auto new_center = cam.center() + right * frameSpeed;
                cam.setCenter(new_center);
            }

            // 🎯 鼠标左键：根据位移平移摄像机（Pan）
            if (state.mouse_button_left.Value() && state.key_down.Value()) {
                float panSpeed = 0.0035f;  // 平移速度

                // 根据鼠标位移计算平移量
                glm::vec3 panDelta =
                    right * state.mouseRelativeX_ * panSpeed +  // 水平移动：鼠标右移 ->
                    up * state.mouseRelativeY_ * panSpeed;      // 垂直移动：鼠标上移

                // 同时移动 eye 和 center，保持视角方向不变

                auto new_eye = cam.eye() + panDelta;
                cam.setEye(new_eye);
                auto new_center = cam.center() + panDelta;
                cam.setCenter(new_center);
            }

            // 🔁 右键旋转视角（如果你还需要旋转功能）
            if (state.key_down.Value() && state.mouse_button_right.Value()) {
                const glm::vec3 WORLD_UP(0, 1, 0);
                glm::vec3 toEye = cam.eye() - cam.center();
                float dist = glm::length(toEye);
                if (dist < 0.1f) return;

                toEye = glm::normalize(toEye);

                float sensitivity = 0.002f;
                float dx = state.mouseRelativeX_ * sensitivity;
                float dy = -state.mouseRelativeY_ * sensitivity;

                // 🔁 1. 先绕 WORLD_UP 旋转（yaw）—— 左右
                glm::mat4 yawRot = glm::rotate(glm::mat4(1.0f), -dx, WORLD_UP);
                toEye = glm::vec3(yawRot * glm::vec4(toEye, 0.0f));

                // 🔁 2. 重新计算 right（因为 toEye 变了）
                glm::vec3 right = glm::cross(toEye, WORLD_UP);
                if (glm::length(right) < 1e-6f) {
                    right = glm::vec3(1, 0, 0);  // fallback
                } else {
                    right = glm::normalize(right);
                }

                // 🔽 3. 绕 right 旋转（pitch）—— 上下
                glm::mat4 pitchRot = glm::rotate(glm::mat4(1.0f), -dy, right);
                glm::vec3 newToEye = glm::vec3(pitchRot * glm::vec4(toEye, 0.0f));

                // ✅ 限制 pitch，防止翻转
                float dot = glm::dot(newToEye, WORLD_UP);
                if (dot > 0.99f || dot < -0.99f) {
                    // 角度太大，拒绝更新
                } else {
                    toEye = newToEye;
                }

                cam.setEye(cam.center() + toEye * dist);
            }

            // 🧮 缩放
            if (state.scrollOffset_ != 0.0f) {
                float dist = glm::distance(cam.eye(), cam.center());
                float newDist = dist * (1.0f - state.scrollOffset_ * 0.1f);
                newDist = glm::clamp(newDist, 0.5f, 50.0f);

                glm::vec3 direction = glm::normalize(cam.eye() - cam.center());
                cam.eye() = cam.center() + direction * newDist;
            }
        }
};

}  // namespace graphics