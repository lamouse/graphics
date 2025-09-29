#pragma once
#include "common/common_funcs.hpp"
#include "ecs/scene/scene.hpp"
#include "ecs/scene/entity.hpp"
namespace world {

    enum class WorldEntityType : std::uint8_t{
        CAMERA
    };

    class World {
        public:
            World();
            [[nodiscard]] auto getEntity(WorldEntityType entityType) const -> ecs::Entity;
            ~World();
            void drawUI();
            CLASS_DEFAULT_MOVEABLE(World);
            CLASS_NON_COPYABLE(World);
        private:
            ecs::Scene scene_;
            ecs::Entity cameraEntity_{};
    };
}  // namespace world