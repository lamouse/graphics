#pragma once
#include "common/common_funcs.hpp"
#include <entt/entt.hpp>

namespace ecs {
class Entity;
class Scene {
    public:
        Scene();
        ~Scene();
        CLASS_DEFAULT_MOVEABLE(Scene);
        CLASS_NON_COPYABLE(Scene);
        auto createEntity() -> Entity;
        auto createEntity(const std::string& tag) -> Entity;

    private:
        entt::registry registry_;
        friend class Entity;
};
}  // namespace ecs