#include "scene.hpp"
#include <glm/glm.hpp>
#include "entity.hpp"
#include "ecs/components/tag_component.hpp"

namespace ecs {

namespace {
void do_math(const glm::mat4& transform) {}
}  // namespace

Scene::Scene() {}

auto Scene::createEntity() -> Entity { return Entity(registry_.create(), this); }

auto Scene::createEntity(std::string tag) -> Entity {
    auto entity = createEntity();
    entity.addComponent<TagComponent>(std::move(tag));
    return entity;
}

Scene::~Scene() {}

}  // namespace ecs