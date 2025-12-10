#pragma once

#include <glm/glm.hpp>

namespace core {

class Camera {
    public:
        void setOrthographicProjection(float left, float right, float top, float bottom, float near,
                                       float far);
        void setPerspectiveProjection(float fovy, float aspect, float near, float far);

        void setViewDirection(glm::vec3 position, glm::vec3 direction,
                              glm::vec3 up = glm::vec3{0.F, 1.F, 0.F});
        void setViewTarget(glm::vec3 position, glm::vec3 target,
                           glm::vec3 up = glm::vec3{0.F, -1.F, 0.F});
        void setViewYXZ(glm::vec3 position, glm::vec3 rotation);

        [[nodiscard]] auto getProjection() const -> const glm::mat4& { return projectionMatrix; }
        [[nodiscard]] auto getView() const -> const glm::mat4& { return viewMatrix; }
        [[nodiscard]] auto getInverseView() const -> const glm::mat4& { return inverseViewMatrix; }
        auto getPosition() -> glm::vec3 { return {inverseViewMatrix[3]}; }
        auto right() { return glm::vec3(inverseViewMatrix[0]); }
        auto left() -> glm::vec3 { return -glm::vec3(inverseViewMatrix[0]); }
        auto up() { return glm::vec3(inverseViewMatrix[1]); }
        auto front() -> glm::vec3 { return glm::vec3(inverseViewMatrix[2]); }

    private:
        glm::mat4 projectionMatrix{1.F};
        glm::mat4 viewMatrix{1.F};
        glm::mat4 inverseViewMatrix{1.F};
};

}  // namespace core