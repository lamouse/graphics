#include "model.hpp"

#include "system/pick_system.hpp"
#include "system/transform_system.hpp"
namespace graphics::effects {

auto uploadMeshMaterialResource(graphics::ResourceManager& manager, const SubMesh& subMesh)
    -> std::tuple<MeshMaterialResource, MaterialUBO> {
    MeshMaterialResource materialResource{};
    MaterialUBO materialUBO{};
    materialUBO.ambient = {subMesh.material.ambientColor, 1.f};
    materialUBO.diffuse = {subMesh.material.diffuseColor, 1.f};
    materialUBO.specular = subMesh.material.specularColor;
    materialUBO.shininess = subMesh.material.shininess;
    if (!subMesh.material.ambientTextures.empty()) {
        materialResource.ambientTextures =
            manager.addKtxTexture(subMesh.material.ambientTextures[0]);
    } else {
        materialResource.ambientTextures =
            manager.getTexture(std::string(DEFAULT_1X1_WRITE_TEXTURE));
    }

    if (!subMesh.material.diffuseTextures.empty()) {
        materialResource.diffuseTextures =
            manager.addKtxTexture(subMesh.material.diffuseTextures[0]);
    } else {
        materialResource.diffuseTextures =
            manager.getTexture(std::string(DEFAULT_1X1_WRITE_TEXTURE));
    }

    if (!subMesh.material.specularTextures.empty()) {
        materialResource.specularTextures =
            manager.addKtxTexture(subMesh.material.specularTextures[0]);
    } else {
        materialResource.specularTextures =
            manager.getTexture(std::string(DEFAULT_1X1_WRITE_TEXTURE));
    }
    if (!subMesh.material.normalTextures.empty()) {
        materialResource.normalTextures = manager.addKtxTexture(subMesh.material.normalTextures[0]);
    } else {
        materialResource.normalTextures =
            manager.getTexture(std::string(DEFAULT_1X1_WRITE_TEXTURE));
    }
    return {materialResource, materialUBO};
}

LightModel::LightModel(graphics::ResourceManager& manager, const layout::FrameBufferLayout& layout,
                       const ModelResourceName& names, const std::string& name)
    : id(getCurrentId()) {
    auto shader_hash = manager.getShaderHash<ShaderHash>(names.shader_name);
    auto mesh_id = manager.getMesh(names.mesh_name);

    auto sub_mesh = manager.getModelSubMesh(mesh_id);
    meshes.reserve(sub_mesh.size());
    for (const auto& mesh : sub_mesh) {
        auto [materialResource, materialUBO] = uploadMeshMaterialResource(manager, mesh);
        meshes.emplace_back(
            render::RenderCommand{
                .indexOffset = mesh.indexOffset,
                .indexCount = mesh.indexCount,
            },
            shader_hash, layout, name + "mesh", mesh_id, materialResource);
        meshes.back().getUBO<MaterialUBO>() = materialUBO;
    }
    mesh_ids.insert(meshes.back().getId());
    auto vertex = manager.getMeshVertex(mesh_id);
    auto indics = manager.getMeshIndics(mesh_id);
    PickingSystem::upload_vertex(id, vertex, indics);
    entity_ = getEffectsScene().createEntity("LightModel" + std::to_string(id));
    entity_.addComponent<ecs::RenderStateComponent>(id);
    entity_.addComponent<ecs::TransformComponent>();
}

void LightModel::update(const core::FrameInfo& frameInfo, world::World& world) {
    ZoneScopedNC("model::update", 110);
    auto& transform = entity_.getComponent<ecs::TransformComponent>();
    auto& render_state = entity_.getComponent<ecs::RenderStateComponent>();

    if (render_state.is_select()) {
        if (frameInfo.input_state.key == core::InputKey::LCtrl) {
            if (frameInfo.input_state.scrollOffset_ != 0.0f) {
                scale(transform, frameInfo.input_state.scrollOffset_);
            }
        }
    }

    auto [pick_id, pick] = world.pick();

    if (pick && mesh_ids.contains(pick_id)) {
        render_state.mouse_select = true;
    } else {
        render_state.mouse_select = false;
    }
    if (render_state.mouse_select) {
        move_model(frameInfo, transform);
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
            dirLight.color = glm::vec4(lightComponent.color, lightComponent.intensity);
            pointLightUbo.dirLight = dirLight;

            // TODO 临时测试
            pointLightUbo.spotLight.position =
                glm::vec4(frameInfo.camera->getPosition(), lightComponent.outerCone);
            pointLightUbo.spotLight.direction =
                glm::vec4(frameInfo.camera->front(), lightComponent.innerCone);
            pointLightUbo.spotLight.color = glm::vec4(lightComponent.color, 1);
            // TODO 临时测试
        }
    }
    auto modelMatrix = transform.mat4();
    auto normalMatrix = transform.normalMatrix();
    pointLightUbo.numLights = index;
    for (auto& mesh : meshes) {
        mesh.PushConstant().modelMatrix = modelMatrix;
        mesh.PushConstant().normalMatrix = normalMatrix;
        mesh.getUBO<LightUBO>() = pointLightUbo;
    }
}

}  // namespace graphics::effects