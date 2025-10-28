#include "particle_instance.hpp"
#include "resource.hpp"
#include "ecs/components/render_state_component.hpp"
namespace graphics {
ParticleInstance::ParticleInstance(const ResourceManager& resource, const std::string& textureName,
                                   const std::string& meshName, std::string shaderName)
    : id(currentId++), texture(textureName), mesh(meshName), shader(std::move(shaderName)) {
    textureId = resource.getTexture(textureName);
    meshId = resource.getMesh(meshName);
    auto hash = resource.getShaderHash<ShaderHash>(shader);
    vertex_shader_hash = hash.vertex;
    fragment_shader_hash = hash.fragment;

    entity_ = scene.createEntity("Particle" + std::to_string(id));
    entity_.addComponent<ecs::RenderStateComponent>(id);
}
}  // namespace graphics