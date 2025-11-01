#include "world.hpp"
#include "ecs/components/camera_component.hpp"
#include "ecs/components/render_state_component.hpp"
#include "ecs/ui/cameraUI.hpp"
#include "resource/id.hpp"
namespace world {

World::World() {
    cameraEntity_ = scene_.createEntity("camera");
    cameraEntity_.addComponent<ecs::CameraComponent>();
    cameraEntity_.addComponent<ecs::RenderStateComponent>(graphics::getCurrentId());
}

[[nodiscard]] auto World::getEntity(WorldEntityType entityType) const -> ecs::Entity {
    switch (entityType) {
        case WorldEntityType::CAMERA:
            return cameraEntity_;
        default:
            throw std::runtime_error("Unknown entity type");
    }
}

World::~World()=default;

}  // namespace world