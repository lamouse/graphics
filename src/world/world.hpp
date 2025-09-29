#pragma once
#include "common/common_funcs.hpp"
#include "ecs/scene/scene.hpp"
namespace world {

    class World {
        public:
            World();
            ~World();
            CLASS_DEFAULT_MOVEABLE(World);
            CLASS_NON_COPYABLE(World);
        private:
            ecs::Scene scene_;

    };
}  // namespace world