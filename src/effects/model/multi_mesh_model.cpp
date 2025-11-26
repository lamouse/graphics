#include "effects/model/multi_mesh_model.hpp"
#include "system/pick_system.hpp"
namespace graphics::effects {
ModelForMultiMesh::ModelForMultiMesh(ResourceManager& manager,
                                     const layout::FrameBufferLayout& layout,
                                     const ModelResourceName& names, const std::string& name)
    : id(getCurrentId()) {
    auto shader_hash = manager.getShaderHash<ShaderHash>(names.shader_name);
    auto model_config = manager.getModelConfig(names.mesh_name);
    MultiMeshModel model(model_config.path, model_config.hash, model_config.flip_uv);
    auto sub_meshes = model.getMeshes();
    for (uint32_t i = 0; const auto& mesh : sub_meshes) {
        auto mesh_id = manager.addMesh(names.mesh_name + "mesh: " + std::to_string(i++), mesh);
        manager.addMeshVertex(mesh_id, mesh.only_vertex, mesh.indices_);

        SubMesh sub_mesh{.material = mesh.material};
        auto [materialResource, materialUBO] = uploadMeshMaterialResource(manager, sub_mesh);
        meshes.emplace_back(
            render::RenderCommand{
                .indexOffset = 0,
                .indexCount = static_cast<uint32_t>(mesh.indices_.size()),
            },
            shader_hash, layout, name + "mesh", mesh_id, materialResource);
        meshes.back().getUBO<MaterialUBO>() = materialUBO;
        PickingSystem::upload_vertex(meshes.back().getId(), mesh.only_vertex, mesh.indices_);
        mesh_ids.insert(meshes.back().getId());
    }
    entity_ = getEffectsScene().createEntity(name + ": " + std::to_string(id));
    entity_.addComponent<ecs::RenderStateComponent>(id);
    entity_.addComponent<ecs::TransformComponent>();
}

void ModelForMultiMesh::update(const core::FrameInfo& frameInfo, world::World& world) {
    ZoneScopedNC("model::update", 110);
    const auto [down, first] = frameInfo.input_state.mouseLeftButtonDown();

    auto& transform = entity_.getComponent<ecs::TransformComponent>();
    auto& render_state = entity_.getComponent<ecs::RenderStateComponent>();

    // transform.rotation.x = 4.8f;

    if (render_state.mouse_select && frameInfo.input_state.mouseLeftButtonUp()) {
        render_state.mouse_select = false;
    }
    if (render_state.mouse_select) {
        // move_model(frameInfo, transform);
        move_model(frameInfo, transform, false, out_initialWorldZ, out_dragStartWorldPos);
    }
    if (render_state.is_select()) {
        if (frameInfo.input_state.key == core::InputKey::LCtrl) {
            if (frameInfo.input_state.scrollOffset_ != 0.0f) {
                auto scale = transform.scale.x;
                scale += frameInfo.input_state.scrollOffset_ * 0.02f;
                if (scale <= 0.01) {
                    scale = 0.01f;
                }
                if (scale > 2.0) {
                    scale = 2.0f;
                }
                transform.scale = glm::vec3(scale);
            }
        }
    }

    if (first) {
        pending_pick_ = true;
    } else {
        if (!render_state.mouse_select && down && pending_pick_ &&
            entity_.getComponent<ecs::RenderStateComponent>().visible) {
            check_pick(mesh_ids, frameInfo, render_state, transform);
        }
        if (render_state.mouse_select) {
            move_model(frameInfo, transform, true, out_initialWorldZ, out_dragStartWorldPos);
        }
        pending_pick_ = false;
    }

    auto light_entity = world.getLightEntities();
    LightUBO pointLightUbo{};
    pointLightUbo.projection = frameInfo.camera->getProjection();
    pointLightUbo.view = frameInfo.camera->getView();
    pointLightUbo.inverseView = frameInfo.camera->getView();
    pointLightUbo.ambientLightColor = glm::vec4{1.f, 1.f, 1.f, .04f};
    int index = 0;
    for (const auto& entity : light_entity) {
        auto& lightComponent = entity.getComponent<ecs::LightComponent>();
        if (lightComponent.type == ecs::LightType::Point) {
            auto& light_transform = entity.getComponent<ecs::TransformComponent>();
            PointLight light{};

            light.color = {lightComponent.color, lightComponent.intensity};
            light.position = {light_transform.translation, 1.0f};
            pointLightUbo.pointLights[index] = light;
            index++;
            if (index >= MAX_LIGHTS) {
                break;
            }
        } else if (lightComponent.type == ecs::LightType::Directional) {
            DirLight dirLight{};
            dirLight.direction = glm::vec4(glm::normalize(lightComponent.direction), 0.f);
            dirLight.color = glm::vec4(lightComponent.color, 0.f) * lightComponent.intensity;
            pointLightUbo.dirLight = dirLight;

            // TODO 临时测试
            pointLightUbo.spotLight.position =
                glm::vec4(frameInfo.camera->getPosition(), lightComponent.outerCone);
            pointLightUbo.spotLight.direction =
                glm::vec4(frameInfo.camera->front(), lightComponent.innerCone);
            pointLightUbo.spotLight.color = glm::vec4(lightComponent.color, 10);
            // TODO 临时测试
        }
    }
    auto modelMatrix = transform.mat4();
    auto normalMatrix = transform.normalMatrix();
    pointLightUbo.numLights = index;
    for (auto& mesh : meshes) {
        PickingSystem::update_transform(mesh.getId(), transform);
        mesh.PushConstant().modelMatrix = modelMatrix;
        mesh.PushConstant().normalMatrix = normalMatrix;
        mesh.getUBO<LightUBO>() = pointLightUbo;
    }
}

}  // namespace graphics::effects