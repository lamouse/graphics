#include "instance.hpp"
namespace graphics {
auto getModelScene() -> ecs::Scene& {
    static ecs::Scene instance;
    return instance;
}
}  // namespace graphics