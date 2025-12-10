#include "instance.hpp"
#include "ecs/scene/scene.hpp"
namespace graphics {
auto getModelScene() -> ecs::Scene& {
    static ecs::Scene instance;
    return instance;
}
}  // namespace graphics