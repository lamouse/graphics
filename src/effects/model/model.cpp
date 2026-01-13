#include "model.hpp"

#include "system/pick_system.hpp"
#include "resource/obj/model_mesh.hpp"

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

LightModel::LightModel(graphics::ResourceManager& manager, const ModelResourceName& names,
                       const std::string& name)
    : id(getCurrentId()) {
    auto shader_hash = manager.getShaderHash<ShaderHash>(names.shader_name);

    auto mesh_id = manager.addModel(names.mesh_name);
    auto sub_mesh = manager.getModelSubMesh(mesh_id);
    materials.reserve(sub_mesh.size());
    meshes.reserve(sub_mesh.size());
    for (const auto& mesh : sub_mesh) {
        auto [materialResource, materialUBO] = uploadMeshMaterialResource(manager, mesh);
        materials.push_back(materialUBO);
        meshes.emplace_back(
            render::RenderCommand{
                .indexOffset = mesh.indexOffset,
                .indexCount = mesh.indexCount,
            },
            shader_hash, name + "mesh", mesh_id, materialResource);
        meshes.back().setUBO(&materials.back());
        meshes.back().setUBO(&light_ubo);
        meshes.back().setPushConstant(&push_constant);
        auto vertex = manager.getMeshVertex(mesh_id);
        auto indics = manager.getMeshIndics(mesh_id);
        PickingSystem::upload_vertex(id, meshes.back().getId(), vertex, indics);
        mesh_ids.insert(meshes.back().getId());
    }

    entity_ = getEffectsScene().createEntity("LightModel" + std::to_string(id));
    entity_.addComponent<ecs::RenderStateComponent>(id);
    entity_.addComponent<ecs::TransformComponent>();
    render_state = &entity_.getComponent<ecs::RenderStateComponent>();  // NOLINT
    transform = &entity_.getComponent<ecs::TransformComponent>();       // NOLINT
}

void LightModel::update(const core::FrameInfo& frameInfo, world::World& world) {
    ZoneScopedNC("model::update", 110);

    updateLightUBO(frameInfo, light_ubo, world);

    push_constant.modelMatrix = transform->mat4();
    push_constant.normalMatrix = transform->normalMatrix();
    PickingSystem::update_transform(id, *transform);
}

}  // namespace graphics::effects