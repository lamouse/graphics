#pragma once
#include <entt/entt.hpp>

namespace ecs {
class Entity;
class Scene {
    public:
        Scene();
        ~Scene();
        Scene(const Scene&) = delete;
        Scene(Scene&&) = default;
        auto operator=(const Scene&)->Scene& = delete;
        auto operator=(Scene&&)->Scene& = default;
        auto createEntity() -> Entity;
        auto createEntity(const std::string& tag) -> Entity;

    private:
        entt::registry registry_;
        friend class Entity;
};
}  // namespace ecs