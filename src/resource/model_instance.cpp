#include "model_instance.hpp"
#include "ecs/ui/transformUI.hpp"
#include "resource/resource.hpp"
#include <imgui.h>
#include "id.hpp"
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

ModelInstance::ModelInstance(const ResourceManager& resource, const std::string& textureName,
                             const std::string& meshName, std::string shaderName)
    : id(getCurrentId()), texture(textureName), mesh(meshName), shader(std::move(shaderName)) {
    textureId = resource.getTexture(textureName);
    meshId = resource.getMesh(meshName);
    auto hash = resource.getShaderHash<ShaderHash>(shader);
    vertex_shader_hash = hash.vertex;
    fragment_shader_hash = hash.fragment;
    entity_ = getModelScene().createEntity("Model" + std::to_string(id));
    entity_.addComponent<ecs::TransformComponent>();
    entity_.addComponent<ecs::RenderStateComponent>(id);
}

}  // namespace graphics