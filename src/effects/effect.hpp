#pragma once
namespace ecs {
class Scene;
}
namespace graphics::effects {
auto getEffectsScene() -> ecs::Scene&;
}