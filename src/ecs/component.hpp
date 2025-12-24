#pragma once
#include "components/transform_component.hpp"
#include "components/light_component.hpp"
#include "components/camera_component.hpp"
#include "components/render_state_component.hpp"
#include "ecs/scene/entity.hpp"
namespace ecs {
struct Outliner {
        Entity entity;
        std::vector<Entity> children;
};

}  // namespace ecs