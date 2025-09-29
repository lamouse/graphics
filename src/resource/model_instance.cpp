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

auto ModelInstance::getImageData() const -> std::unique_ptr<resource::image::Image> {
    if(image_path.empty()){
        return nullptr;
    }
    return std::make_unique<resource::image::Image>(image_path);
}

}  // namespace graphics