#pragma once
#include "common/common_funcs.hpp"
#include "ecs/scene/scene.hpp"
#include "ecs/scene/entity.hpp"
#include <vector>
namespace world {

enum class WorldEntityType : std::uint8_t { CAMERA };

class World {
    public:
        World();
        [[nodiscard]] auto getEntity(WorldEntityType entityType) const -> ecs::Entity;
        void addLightEntity(const ecs::Entity& entity);
        [[nodiscard]] auto getLightEntities(this auto&& self) -> decltype(auto) {
            {
                return (std::span(self.lightEntity_));
            }
        }
        [[nodiscard]] auto getScene() -> ecs::Scene&;
        ~World();
        CLASS_DEFAULT_MOVEABLE(World);
        CLASS_NON_COPYABLE(World);
        void cleanLight() { lightEntity_.clear(); }

    private:
        ecs::Scene scene_;
        ecs::Entity cameraEntity_;
        std::vector<ecs::Entity> lightEntity_;
};
}  // namespace world