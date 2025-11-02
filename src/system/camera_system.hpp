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
            glm::vec3 forward = glm::normalize(cam.center - cam.eye);
            glm::vec3 right = glm::normalize(glm::cross(cam.up, forward));
            glm::vec3 up = glm::normalize(glm::cross(forward, right));

            float frameSpeed = moveSpeed * deltaTime;

            // 🚶 WASD 移动
            if (state.key == core::InputKey::W && state.key_down.Value()) {
                cam.eye += forward * frameSpeed;
                cam.center += forward * frameSpeed;
            }
            if (state.key == core::InputKey::S && state.key_down.Value()) {
                cam.eye -= forward * frameSpeed;
                cam.center -= forward * frameSpeed;
            }
            if (state.key == core::InputKey::A && state.key_down.Value()) {
                cam.eye -= right * frameSpeed;
                cam.center -= right * frameSpeed;
            }
            if (state.key == core::InputKey::D && state.key_down.Value()) {
                cam.eye += right * frameSpeed;
                cam.center += right * frameSpeed;
            }


            // 🎯 鼠标左键：根据位移平移摄像机（Pan）
            if (state.mouse_button_left.Value() && state.key_down.Value()) {
                float panSpeed = 0.0035f;  // 平移速度

                // 根据鼠标位移计算平移量
                glm::vec3 panDelta =
                    right * state.mouseRelativeX_ * panSpeed +  // 水平移动：鼠标右移 ->
                     up * state.mouseRelativeY_ * panSpeed;       // 垂直移动：鼠标上移

                // 同时移动 eye 和 center，保持视角方向不变
                cam.eye += panDelta;
                cam.center += panDelta;
            }

            // // 🔁 右键旋转视角（如果你还需要旋转功能）
            // if (state.key_down.Value() && state.mouse_button_right.Value()) {
            //     float yaw = state.mouseRelativeX_ * 0.005f;
            //     float pitch = state.mouseRelativeY_ * 0.005f;

            //     glm::vec3 toEye = cam.eye - cam.center;
            //     glm::mat4 yawRot = glm::rotate(glm::mat4(1.0f), yaw, up);
            //     toEye = glm::vec3(yawRot * glm::vec4(toEye, 0.0f));

            //     glm::mat4 pitchRot = glm::rotate(glm::mat4(1.0f), pitch, right);
            //     glm::vec3 newToEye = glm::vec3(pitchRot * glm::vec4(toEye, 0.0f));

            //     glm::vec3 newForward = -glm::normalize(newToEye);
            //     float angle = glm::acos(glm::dot(newForward, up));
            //     if (angle > 0.1f && angle < 3.0f) {
            //         toEye = newToEye;
            //     }

            //     cam.eye = cam.center + toEye;
            // }

            // 🧮 缩放
            if (state.scrollOffset_ != 0.0f) {
                float dist = glm::distance(cam.eye, cam.center);
                float newDist = dist * (1.0f - state.scrollOffset_ * 0.1f);
                newDist = glm::clamp(newDist, 0.5f, 50.0f);

                glm::vec3 direction = glm::normalize(cam.eye - cam.center);
                cam.eye = cam.center + direction * newDist;
            }

        }
};

}  // namespace graphics