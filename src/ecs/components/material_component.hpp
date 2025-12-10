#pragma once
#include <glm/glm.hpp>
namespace ecs {
struct MaterialComponent {
        glm::vec3 ambient{};
        glm::vec3 diffuse{};
        glm::vec3 specular{};
        bool useTexture{true};
        bool useLighting{true};
        bool useNormalMap{false};
        bool useSpecularMap{false};
        float shininess{32.f};
};
}  // namespace ecs