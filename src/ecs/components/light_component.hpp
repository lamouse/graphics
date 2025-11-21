#pragma once
#include <glm/glm.hpp>
namespace ecs {
enum class LightType : uint32_t {
    Point = 0,        // 点光源：有位置，有范围，向各方向发光
    Directional = 1,  // 平行光：只有方向，没有位置，常用于太阳光
    Spot = 2          // 聚光灯：有位置和方向，带内外锥角
};
struct LightComponent {
        LightType type{LightType::Point};
        glm::vec3 color{1.f};
        float intensity{1.f};
        float range{10.0f};  // 点光源衰减范围

        glm::vec3 direction{0.0f, -1.0f, 0.0f};  // 平行光/聚光灯方向

        float innerCone{glm::cos(glm::radians(15.0f))};  // 内锥角 (存 cos)
        float outerCone{glm::cos(glm::radians(30.0f))};  // 外锥角 (存 cos)
};

}  // namespace ecs