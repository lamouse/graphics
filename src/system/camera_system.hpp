#pragma once
#include "ecs/components/camera_component.hpp"
#include "core/input.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace graphics {
    namespace input {
        class InputSystem;

    }

struct CameraSystem {
    public:
        static void update(ecs::CameraComponent& cam, input::InputSystem* input, core::InputState state, float deltaTime);
};

}  // namespace graphics