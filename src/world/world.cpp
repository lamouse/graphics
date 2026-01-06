#include "world.hpp"
#include "core/frame_info.hpp"
#include "ecs/components/render_state_component.hpp"
#include "ecs/component.hpp"
#include "input/input.hpp"
#include "input/mouse.h"
#include "core/frontend/window.hpp"
#include "resource/id.hpp"
#include "resource/resource.hpp"
#include "render_core/mesh.hpp"

#include "system/camera_system.hpp"
#include "system/pick_system.hpp"
#include "system/transform_system.hpp"

namespace world {

World::World() : id_(graphics::getCurrentId()), frame_time_(std::make_unique<core::FrameTime>()) {
    cameraEntity_ = scene_.createEntity("camera");
    cameraEntity_.addComponent<ecs::CameraComponent>();
    cameraEntity_.addComponent<ecs::RenderStateComponent>(graphics::getCurrentId());
    cameraComponent_ = &cameraEntity_.getComponent<ecs::CameraComponent>();  // NOLINT
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

void World::update(core::frontend::BaseWindow& window, graphics::ResourceManager& resourceManager,
                   graphics::input::InputSystem& input_system) {
    cameraComponent_->setAspect(window.getAspectRatio());
    core::FrameInfo frameInfo;
    frameInfo.frame_layout = window.getFramebufferLayout();
    frameInfo.frame_time = frame_time_->get();
    frameInfo.resource_manager = &resourceManager;
    auto& camera = cameraComponent_->getCamera();
    frameInfo.camera = &camera;
    graphics::CameraSystem::update(*cameraComponent_, &input_system,
                                   static_cast<float>(frameInfo.frame_time.frame));
    process_mouse_input(frameInfo, input_system.GetMouse());

    render_registry_.updateAll(frameInfo, *this);
}

void World::process_mouse_input(core::FrameInfo& frameInfo, graphics::input::Mouse* mouse) {
    if (mouse->IsPressed(graphics::input::MouseButton::Left)) {
        auto mouse_axis = mouse->GetAxis();

        if (!is_pick) {
            auto origin = mouse->GetMouseOrigin();
            auto pick_result = graphics::PickingSystem::pick(
                *frameInfo.camera, origin.x, origin.y, static_cast<float>(frameInfo.frame_layout.screen.GetWidth()),
                static_cast<float>(frameInfo.frame_layout.screen.GetHeight()));
            if (pick_result) {
                this->pick(pick_result->model_id, pick_result->id);
            }
            is_pick = true;
        } else {
            auto* draw_able = render_registry_.getDrawableById(pick_id);
            if (draw_able && draw_able->getEntity().hasComponent<ecs::TransformComponent>()) {
                auto& transform = draw_able->getEntity().getComponent<ecs::TransformComponent>();
                graphics::move_model(frameInfo, transform);
            }
        }

    } else {
        this->cancelPick();

        is_pick = false;
    }
}

void World::draw(render::Graphic* gfx) { render_registry_.drawAll(gfx); }

}  // namespace world