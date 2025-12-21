#include "world.hpp"
#include "core/frame_info.hpp"
#include "ecs/components/render_state_component.hpp"
#include "ecs/component.hpp"
#include "resource/id.hpp"
#include "system/camera_system.hpp"

namespace world {

World::World() : id_(graphics::getCurrentId()) {
    cameraEntity_ = scene_.createEntity("camera");
    cameraEntity_.addComponent<ecs::CameraComponent>();
    cameraEntity_.addComponent<ecs::RenderStateComponent>(graphics::getCurrentId());
    dirLightEntity_ = scene_.createEntity("dir_light");
    entity_ = scene_.createEntity("world: " + std::to_string(id_));
    entity_.addComponent<ecs::RenderStateComponent>(id_);
    ecs::LightComponent dirLight{};
    dirLight.type = ecs::LightType::Directional;
    dirLight.color = glm::vec3{1.f, 1.f, 1.f};
    dirLight.intensity = 0.2f;
    dirLight.direction = glm::vec3{-0.2f, -1.0f, -0.3f};
    dirLightEntity_.addComponent<ecs::LightComponent>(dirLight);
    dirLightEntity_.addComponent<ecs::RenderStateComponent>(graphics::getCurrentId());
    dir_light = &dirLightEntity_.getComponent<ecs::LightComponent>();  // NOLINT
    lights_.push_back({.light = dir_light, .transform = nullptr});
    child_entitys_ = {cameraEntity_, dirLightEntity_};
}

[[nodiscard]] auto World::getEntity(WorldEntityType entityType) const -> ecs::Entity {
    switch (entityType) {
        case WorldEntityType::CAMERA:
            return cameraEntity_;
        default:
            throw std::runtime_error("Unknown entity type");
    }
}

[[nodiscard]] auto World::getScene() -> ecs::Scene& { return scene_; }

World::~World() = default;

void World::update(const core::FrameInfo& frameInfo) {
    if (frameInfo.input_event) {
        auto& cameraComponent = cameraEntity_.getComponent<ecs::CameraComponent>();
        graphics::CameraSystem::update(cameraComponent, frameInfo.input_event.value(),
                                       frameInfo.frameTime);
    }
    render_registry_.updateAll(frameInfo, *this);
}

void World::draw(render::Graphic* gfx) { render_registry_.drawAll(gfx); }

}  // namespace world