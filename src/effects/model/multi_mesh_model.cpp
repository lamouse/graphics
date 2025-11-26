#include "effects/model/multi_mesh_model.hpp"
#include "system/pick_system.hpp"
#include "system/transform_system.hpp"
namespace graphics::effects {
ModelForMultiMesh::ModelForMultiMesh(ResourceManager& manager,
                                     const layout::FrameBufferLayout& layout,
                                     const ModelResourceName& names, const std::string& name)
    : id(getCurrentId()) {
    auto shader_hash = manager.getShaderHash<ShaderHash>(names.shader_name);
    auto model_config = manager.getModelConfig(names.mesh_name);
    MultiMeshModel model(model_config.path, model_config.hash, model_config.flip_uv);
    auto sub_meshes = model.getMeshes();
    materials.reserve(sub_meshes.size());
    for (uint32_t i = 0; const auto& mesh : sub_meshes) {
        auto mesh_id = manager.addMesh(names.mesh_name + "mesh: " + std::to_string(i++), mesh);
        manager.addMeshVertex(mesh_id, mesh.only_vertex, mesh.indices_);

        SubMesh sub_mesh{.material = mesh.material};
        auto [materialResource, materialUBO] = uploadMeshMaterialResource(manager, sub_mesh);
        materials.push_back(materialUBO);
        meshes.emplace_back(
            render::RenderCommand{
                .indexOffset = 0,
                .indexCount = static_cast<uint32_t>(mesh.indices_.size()),
            },
            shader_hash, layout, name + "mesh", mesh_id, materialResource);
        meshes.back().setUBO(&materials.back());
        meshes.back().setUBO(&light_ubo);
        PickingSystem::upload_vertex(meshes.back().getId(), mesh.only_vertex, mesh.indices_);
        mesh_ids.insert(meshes.back().getId());
    }
    entity_ = getEffectsScene().createEntity(name + ": " + std::to_string(id));
    entity_.addComponent<ecs::RenderStateComponent>(id);
    render_state = &entity_.getComponent<ecs::RenderStateComponent>();
    entity_.addComponent<ecs::TransformComponent>();
}

void ModelForMultiMesh::update(const core::FrameInfo& frameInfo, world::World& world) {
    ZoneScopedNC("model::update", 110);

    auto& transform = entity_.getComponent<ecs::TransformComponent>();
    auto [pick_id, pick] = world.pick();

    if (pick && mesh_ids.contains(pick_id)) {
        render_state->mouse_select = true;
    } else {
        render_state->mouse_select = false;
    }
    if (render_state->mouse_select) {
        move_model(frameInfo, transform);
    }

    if (render_state->is_select()) {
        if (frameInfo.input_state.key == core::InputKey::LCtrl) {
            if (frameInfo.input_state.scrollOffset_ != 0.0f) {
                scale(transform, frameInfo.input_state.scrollOffset_);
            }
        }
    }

    auto light_entity = world.getLightEntities();
    light_ubo.projection = frameInfo.camera->getProjection();
    light_ubo.view = frameInfo.camera->getView();
    light_ubo.inverseView = frameInfo.camera->getInverseView();
    light_ubo.ambientLightColor = glm::vec4{1.f, 1.f, 1.f, .04f};
    int index = 0;
    for (const auto& entity : light_entity) {
        auto& lightComponent = entity.getComponent<ecs::LightComponent>();
        if (lightComponent.type == ecs::LightType::Point) {
            auto& light_transform = entity.getComponent<ecs::TransformComponent>();
            PointLight light{};

            light.color = {lightComponent.color, lightComponent.intensity};
            light.position = {light_transform.translation, 1.0f};
            light_ubo.pointLights[index] = light;
            index++;
            if (index >= MAX_LIGHTS) {
                break;
            }
        } else if (lightComponent.type == ecs::LightType::Directional) {
            DirLight dirLight{};
            dirLight.direction = glm::vec4(glm::normalize(lightComponent.direction), 0.f);
            dirLight.color = glm::vec4(lightComponent.color, 0.f) * lightComponent.intensity;
            light_ubo.dirLight = dirLight;

            // TODO 临时测试
            light_ubo.spotLight.position =
                glm::vec4(frameInfo.camera->getPosition(), lightComponent.outerCone);
            light_ubo.spotLight.direction =
                glm::vec4(frameInfo.camera->front(), lightComponent.innerCone);
            light_ubo.spotLight.color = glm::vec4(lightComponent.color, 1);
            // TODO 临时测试
        }
    }
    auto modelMatrix = transform.mat4();
    auto normalMatrix = transform.normalMatrix();
    light_ubo.numLights = index;
    for (auto& mesh : meshes) {
        PickingSystem::update_transform(mesh.getId(), transform);
        mesh.PushConstant().modelMatrix = modelMatrix;
        mesh.PushConstant().normalMatrix = normalMatrix;
    }
}

}  // namespace graphics::effects