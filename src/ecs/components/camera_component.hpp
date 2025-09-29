#pragma once
#include <glm/glm.hpp>
#include "common/common_funcs.hpp"
#include "ecs/camera/camera.hpp"
namespace ecs {
struct CameraComponent {
        ::glm::vec3 lookAt{0.F, 0.F, -5.F};
        ::glm::vec3 center{0.F, 0.F, 0.F};
        ::glm::vec3 up{0.F, 0.F, 0.F};
        float near{10.F};
        float far{.1F};
        float fovy{45.F};
        float extentAspectRation{};
        CameraComponent() = default;
        ~CameraComponent() = default;
        CLASS_DEFAULT_COPYABLE(CameraComponent);
        CLASS_DEFAULT_MOVEABLE(CameraComponent);
        auto setLookAt(const ::glm::vec3& v) -> CameraComponent& {
            lookAt = v;
            return *this;
        }

        auto setCenter(const ::glm::vec3& v) -> CameraComponent& {
            center = v;
            return *this;
        }

        auto setUp(const ::glm::vec3& v) -> CameraComponent& {
            up = v;
            return *this;
        }

        auto setNear(float v) -> CameraComponent& {
            near = v;
            return *this;
        }

        auto setFar(float v) -> CameraComponent& {
            far = v;
            return *this;
        }

        auto setFovy(float v) -> CameraComponent& {
            fovy = v;
            return *this;
        }

        auto setAspect(float v) -> CameraComponent& {
            extentAspectRation = v;
            return *this;
        }
        [[nodiscard]] auto getCamera() const -> Camera {
            Camera camera;
            camera.setPerspectiveProjection(glm::radians(fovy), extentAspectRation, far, near);
            camera.setViewTarget(lookAt, center, up);
            return camera;
        }
};
}  // namespace ecs