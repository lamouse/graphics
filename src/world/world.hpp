#pragma once
#include "common/common_funcs.hpp"
#include "resource/id.hpp"
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
        void cleanLight() { lightEntity_.clear(); lightEntity_.push_back(dirLightEntity_); }
        [[nodiscard]] auto getEntities() const -> std::vector<ecs::Entity> { return std::vector{cameraEntity_, dirLightEntity_}; }
        ecs::Entity entity_;

    private:
        id_t id_;
        ecs::Scene scene_;
        ecs::Entity cameraEntity_;
        ecs::Entity dirLightEntity_;
        std::vector<ecs::Entity> lightEntity_;
};
}  // namespace world