#pragma once
namespace ecs {
class Scene;
}
namespace graphics::effects {

enum class EffectType{
    Model,
    Light,
    CubeMap,
    Particle
};

auto getEffectsScene() -> ecs::Scene&;
}