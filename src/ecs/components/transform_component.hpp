#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "common/common_funcs.hpp"
namespace ecs {
struct TransformComponent {
        ::glm::vec3 translation{};  // position offset
        ::glm::vec3 scale{1.F};
        ::glm::vec3 rotation{};

        [[nodiscard]] auto mat4() const -> ::glm::mat4 {
            auto transform = ::glm::translate(::glm::mat4(1.F), translation);
            transform = ::glm::rotate(transform, rotation.y, {0.F, 1.F, 0.F});
            transform = ::glm::rotate(transform, rotation.x, {1.F, 0.F, 0.F});
            transform = ::glm::rotate(transform, rotation.z, {0.F, 0.F, 1.F});
            transform = ::glm::scale(transform, scale);
            return transform;
        }
        TransformComponent() = default;
        ~TransformComponent() = default;
        CLASS_DEFAULT_COPYABLE(TransformComponent);
        CLASS_DEFAULT_MOVEABLE(TransformComponent);
        explicit TransformComponent(const glm::vec3& translation_,
                                    const ::glm::vec3& scale_ = ::glm::vec3(1.F),
                                    const ::glm::vec3& rotation_ = ::glm::vec3(.0f))
            : translation(translation_), scale(scale_), rotation(rotation_) {}
        // NOLINTNEXTLINE(readability-make-member-function-const, hicpp-explicit-conversions)
        operator glm::mat4() { return mat4(); }
};
}