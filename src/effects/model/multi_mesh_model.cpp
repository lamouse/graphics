#include "effects/model/multi_mesh_model.hpp"
namespace graphics::effects {
ModelForMultiMesh::ModelForMultiMesh(ResourceManager& manager, const layout::FrameBufferLayout& layout,
                          const ModelResourceName& names, const std::string& name)
    : id(getCurrentId()) {
    auto shader_hash = manager.getShaderHash<ShaderHash>(names.shader_name);

    MultiMeshModel model("models/" + names.mesh_name);
    auto sub_meshes = model.getMeshes();
    for (uint32_t i = 0; const auto& mesh : sub_meshes) {
        auto mesh_id = manager.addMesh(names.mesh_name + "mesh: " + std::to_string(i++), mesh);
        SubMesh sub_mesh{.material = mesh.material};
        auto [materialResource, materialUBO] = uploadMeshMaterialResource(manager, sub_mesh);
        meshes.emplace_back(
            render::RenderCommand{
                .indexOffset = 0,
                .indexCount = static_cast<uint32_t>(mesh.indices_.size()),
            },
            shader_hash, layout, name + "mesh", mesh_id, materialResource);
        meshes.back().getUBO<MaterialUBO>() = materialUBO;
    }
    entity_ = getEffectsScene().createEntity(name + ": " + std::to_string(id));
    entity_.addComponent<ecs::RenderStateComponent>(id);
    entity_.addComponent<ecs::TransformComponent>();
}
}  // namespace graphics::effects