#pragma once
#include "common/common_funcs.hpp"
#include <glm/glm.hpp>
#include <span>
namespace world {
class World;
}

namespace core {
struct FrameInfo;
}

namespace graphics::effects {
constexpr auto MAX_LIGHTS = 10;

struct PointLight {
        glm::vec4 position{};  // ignore w
        glm::vec4 color{};     // w is intensity
        CLASS_DEFAULT_COPYABLE(PointLight);
        CLASS_DEFAULT_MOVEABLE(PointLight);
        PointLight() = default;
        ~PointLight() = default;
};

struct DirLight {
        glm::vec4 direction{-0.2f, -1.0f, -0.3f, 0.f};  // ignore w
        glm::vec4 color{1.f, 1.0f, 1.f, 0.2f};          // w is intensity
};
struct SpotLight {
        glm::vec4 position{};   // w used as type
        glm::vec4 color{};      // w is intensity
        glm::vec4 direction{};  // w constant
};

struct LightUBO {
        glm::mat4 projection{1.f};
        glm::mat4 view{1.f};
        glm::mat4 inverseView{1.f};
        glm::vec4 ambientLightColor{1.f, 1.f, 1.f, .02f};  // w is intensity
        DirLight dirLight{};
        SpotLight spotLight{};
        PointLight pointLights[MAX_LIGHTS];
        int numLights{};
        AS_BYTE_SPAN
};

void updateLightUBO(const core::FrameInfo& frameInfo, LightUBO& lightUBO, world::World& world);
}  // namespace graphics::effects