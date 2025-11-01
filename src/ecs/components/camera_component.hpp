#pragma once
#include <glm/glm.hpp>
#include "common/common_funcs.hpp"
#include "core/camera/camera.hpp"
namespace ecs {
constexpr auto DEFAULT_FOVY = 45.F;
constexpr auto DEFAULT_NEAR = 0.1F;
constexpr auto DEFAULT_FAR = 10.F;
constexpr glm::vec3 DEFAULT_UP{.0F, 1.F, 0.F};
constexpr glm::vec3 DEFAULT_EYE{0.F, .0F, 5.0F};
constexpr glm::vec3 DEFAULT_CENTER{.0F, .0F, .0F};
struct CameraComponent {
        ::glm::vec3 eye{DEFAULT_EYE};
        ::glm::vec3 center{DEFAULT_CENTER};
        ::glm::vec3 up{DEFAULT_UP};
        float z_near{DEFAULT_NEAR};
        float z_far{DEFAULT_FAR};
        float fovy{DEFAULT_FOVY};
        float extentAspectRation{};
        CameraComponent() = default;
        ~CameraComponent() = default;
        CLASS_DEFAULT_COPYABLE(CameraComponent);
        CLASS_DEFAULT_MOVEABLE(CameraComponent);
        auto setEye(const ::glm::vec3& v) -> CameraComponent& {
            eye = v;
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
            z_near = v;
            return *this;
        }

        auto setFar(float v) -> CameraComponent& {
            z_far = v;
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
        [[nodiscard]] auto getCamera() const -> core::Camera {
            core::Camera camera;
            camera.setPerspectiveProjection(glm::radians(fovy), extentAspectRation, z_near, z_far);
            camera.setViewTarget(eye, center, up);
            return camera;
        }
};
}  // namespace ecs