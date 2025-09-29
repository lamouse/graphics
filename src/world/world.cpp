#include "world.hpp"
#include "ecs/components/camera_component.hpp"
#include "ecs/ui/cameraUI.hpp"
#include "ecs/ui/tagUI.hpp"
namespace world {

World::World() {
    cameraEntity_ = scene_.createEntity("camera");
    cameraEntity_.addComponent<ecs::CameraComponent>();
}

[[nodiscard]] auto World::getEntity(WorldEntityType entityType) const -> ecs::Entity {
    switch (entityType) {
        case WorldEntityType::CAMERA:
            return cameraEntity_;
        default:
            throw std::runtime_error("Unknown entity type");
    }
}
void World::drawUI() {
    ImGui::Begin("World");
    if (ImGui::TreeNode("Camera")) {
        if (cameraEntity_.hasComponent<ecs::CameraComponent>()) {
            auto& cam = cameraEntity_.getComponent<ecs::CameraComponent>();
            ecs::DrawCameraUI(cam);
        }
        ImGui::TreePop();
    }
    ImGui::End();
}
World::~World() {}

}  // namespace world