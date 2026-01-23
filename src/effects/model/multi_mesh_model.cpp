#include "effects/model/multi_mesh_model.hpp"
#include "system/pick_system.hpp"
#include "resource/obj/model_mesh.hpp"
namespace graphics::effects {
ModelForMultiMesh::ModelForMultiMesh(ResourceManager& manager, const ModelResourceName& names,
                                     const std::string& name)
    : id(getCurrentId()) {
    auto shader_hash = manager.getShaderHash<ShaderHash>(names.shader_name);
    auto model_config = manager.getModelConfig(names.mesh_name);
    MultiMeshModel model(model_config.path, model_config.hash, model_config.flip_uv);
    auto sub_meshes = model.getMeshes();
    materials.reserve(sub_meshes.size());
    child_entitys_.reserve(sub_meshes.size());
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
            shader_hash, sub_mesh.material.name, mesh_id, materialResource);
        meshes.back().setUBO(&materials.back());
        meshes.back().setUBO(&light_ubo);
        meshes.back().setPushConstant(&push_constant);
        PickingSystem::upload_vertex(id, meshes.back().getId(), mesh.only_vertex, mesh.indices_);
        mesh_ids.insert(meshes.back().getId());
        child_entitys_.push_back(meshes.back().entity_);
    }
    entity_ = getEffectsScene().createEntity(name + ": " + std::to_string(id));
    entity_.addComponent<ecs::RenderStateComponent>(id);
    render_state = &entity_.getComponent<ecs::RenderStateComponent>();
    entity_.addComponent<ecs::TransformComponent>();
    transform = &entity_.getComponent<ecs::TransformComponent>();
}

void ModelForMultiMesh::update(const core::FrameInfo& frameInfo, world::World& world) {
    ZoneScopedNC("model::update", 110);

    updateLightUBO(frameInfo, light_ubo, world);
    push_constant.modelMatrix = transform->mat4();
    push_constant.normalMatrix = glm::inverseTranspose(glm::mat3(push_constant.modelMatrix));
    for (auto& mesh : meshes) {
        PickingSystem::update_transform(mesh.getId(), *transform);
    }
}

}  // namespace graphics::effects