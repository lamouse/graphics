#include "effects/effect.hpp"
#include "ecs/scene/scene.hpp"
namespace graphics::effects {
auto getEffectsScene() -> ecs::Scene& {
    static ecs::Scene instance;
    return instance;
}
}  // namespace graphics::effects