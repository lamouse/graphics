#include "particle.hpp"

namespace graphics::effects {
 auto getEffectsScene() -> ecs::Scene& {
        static ecs::Scene instance;
        return instance;
    }
}  // namespace graphic::effects