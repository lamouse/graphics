#pragma once
#include <glm/glm.hpp>
#include "common/common_funcs.hpp"
#include "core/camera/camera.hpp"
namespace ecs {
constexpr auto DEFAULT_FOVY = 45.F;
constexpr auto DEFAULT_NEAR = 0.1F;
constexpr auto DEFAULT_FAR = 100.F;
constexpr auto DEFAULT_SPEED = 5.5f;
constexpr auto DEFAULT_SENSITIVITY = 0.02f;

constexpr glm::vec3 DEFAULT_UP{.0F, 1.F, 0.F};
constexpr glm::vec3 DEFAULT_EYE{0.F, .0F, 5.0F};
constexpr glm::vec3 DEFAULT_CENTER{.0F, .0F, .0F};
struct CameraComponent {
    private:
        ::glm::vec3 eye_{DEFAULT_EYE};
        ::glm::vec3 center_{DEFAULT_CENTER};
        ::glm::vec3 up_{DEFAULT_UP};
        float z_near_{DEFAULT_NEAR};
        float z_far_{DEFAULT_FAR};
        float fovy_{DEFAULT_FOVY};
        float extentAspectRation_{};

        mutable core::Camera camera;
        // ✅ 新增：脏标记
        mutable bool viewDirty{true};
        mutable bool projDirty{true};

    public:
        float speed{DEFAULT_SPEED};
        float sensitivity{DEFAULT_SENSITIVITY};
        CameraComponent() = default;
        ~CameraComponent() = default;
        CLASS_DEFAULT_COPYABLE(CameraComponent);
        CLASS_DEFAULT_MOVEABLE(CameraComponent);
        [[nodiscard]] auto eye() const -> glm::vec3 { return eye_; }
        [[nodiscard]] auto center() const -> glm::vec3 { return center_; }
        [[nodiscard]] auto up() const -> glm::vec3 { return up_; }
        auto getNear() const -> float { return z_near_; }
        auto getFar() const -> float { return z_far_; }
        auto getFovy() const -> float { return fovy_; }
        auto aspect() const -> float { return extentAspectRation_; }

        // ✅ 重写所有会改变相机状态的 setter，标记为 dirty 并更新
        auto setEye(const ::glm::vec3& v) -> CameraComponent& {
            if (eye_ != v) {  // 避免不必要的标记
                eye_ = v;
                viewDirty = true;  // 视图矩阵需要更新
            }
            return *this;
        }

        auto setCenter(const ::glm::vec3& v) -> CameraComponent& {
            if (center_ != v) {
                center_ = v;
                viewDirty = true;
            }
            return *this;
        }

        auto setUp(const ::glm::vec3& v) -> CameraComponent& {
            if (up_ != v) {
                up_ = v;
                viewDirty = true;
            }
            return *this;
        }

        auto setNear(float v) -> CameraComponent& {
            if (z_near_ != v) {
                z_near_ = v;
                projDirty = true;
            }
            return *this;
        }

        auto setFar(float v) -> CameraComponent& {
            if (z_far_ != v) {
                z_far_ = v;
                projDirty = true;
            }
            return *this;
        }

        auto setFovy(float v) -> CameraComponent& {
            if (fovy_ != v) {
                fovy_ = v;
                projDirty = true;
            }
            return *this;
        }

        auto setAspect(float v) -> CameraComponent& {
            if (extentAspectRation_ != v) {
                extentAspectRation_ = v;
                projDirty = true;
            }
            return *this;
        }

        // ✅ 关键：延迟更新函数
        void updateCamera() const {
            if (projDirty) {
                camera.setPerspectiveProjection(glm::radians(fovy_), extentAspectRation_, z_near_,
                                                z_far_);
                projDirty = false;
            }
            if (viewDirty) {
                camera.setViewTarget(eye_, center_, up_);
                viewDirty = false;
            }
            // 如果两个都不 dirty，什么都不做，直接复用旧的 matrix
        }

        // ✅ 改写 getCamera()
        [[nodiscard]] auto getCamera() const -> const core::Camera& {
            updateCamera();  // 只有在 dirty 时才会真正更新
            return camera;
        }
};

}  // namespace ecs