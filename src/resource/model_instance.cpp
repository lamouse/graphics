#include "model_instance.hpp"
#include "ecs/ui/transformUI.hpp"
#include <imgui.h>
namespace graphics {
void ModelInstance::drawUI() {
    ImGui::Begin("Detail");
    if (ImGui::TreeNode("TransformComponent")) {
        if (entity_.hasComponent<ecs::TransformComponent>()) {
            auto& tc = entity_.getComponent<ecs::TransformComponent>();
            ecs::DrawTransformUI(tc);
        }
        ImGui::TreePop();
    }
    ImGui::End();
}

void ModelInstance::updateViewProjection(const ecs::Camera& camera) {
    auto& transform = entity_.getComponent<ecs::TransformComponent>();
    ubo.model = transform.mat4();
    ubo.view = camera.getView();
    ubo.proj = camera.getProjection();
}

}  // namespace graphics