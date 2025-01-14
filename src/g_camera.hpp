#pragma once

#include <glm/glm.hpp>

namespace g {

class Camera {
    public:
        void setOrthographicProjection(float left, float right, float top, float bottom, float near,
                                       float far);
        void setPerspectiveProjection(float fovy, float aspect, float near, float far);

        void setViewDirection(glm::vec3 position, glm::vec3 direction,
                              glm::vec3 up = glm::vec3{0.f, -1.f, 0.f});
        void setViewTarget(glm::vec3 position, glm::vec3 target,
                           glm::vec3 up = glm::vec3{0.f, -1.f, 0.f});
        void setViewYXZ(glm::vec3 position, glm::vec3 rotation);

        [[nodiscard]] auto getProjection() const -> const glm::mat4& { return projectionMatrix; }
        [[nodiscard]] auto getView() const -> const glm::mat4& { return viewMatrix; }
        [[nodiscard]] auto getInverseView() const -> const glm::mat4& { return inverseViewMatrix; }
        auto getPosition() -> glm::vec3 { return {inverseViewMatrix[3]}; }

    private:
        glm::mat4 projectionMatrix{1.f};
        glm::mat4 viewMatrix{1.f};
        glm::mat4 inverseViewMatrix{1.f};
};

}  // namespace g