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

auto ModelInstance::getMeshData() const -> std::unique_ptr<IMeshData> {
    return graphics::Model::createFromFile(mode_path);
}

void ModelInstance::writeToUBOMapData(std::span<const std::byte> data) {
    // NOLINTNEXTLINE
    ASSERT_MSG(!data.empty(), "ubo data size is empty");
    ubo_data = data;
}

}  // namespace graphics