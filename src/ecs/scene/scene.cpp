#include "scene.hpp"
#include <glm/glm.hpp>
#include "entity.hpp"
#include "ecs/components/tag_component.hpp"

namespace ecs {
Scene::Scene() = default;
auto Scene::createEntity() -> Entity { return Entity(registry_.create(), this); }

auto Scene::createEntity(const std::string& tag) -> Entity {
    auto entity = createEntity();
    entity.addComponent<TagComponent>(tag);
    return entity;
}

Scene::~Scene() = default;

}  // namespace ecs