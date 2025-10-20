#include "resource/resource.hpp"
#include "common/assert.hpp"
#include "resource/obj/model.hpp"
#include "resource/shader/shader.hpp"
namespace graphics {
void ResourceManager::addTexture(std::string textureName, add_texture_func func) {
    ASSERT_MSG(!textureName.empty(), "textureName is null");
    ASSERT_MSG(func, "add_texture_func is null");
    resource::image::Image texture(textureName);
    auto textureId = func(texture);
    ASSERT_MSG(!textures.contains(textureName), textureName + " texture in catch");
    textures[textureName] = textureId;
}

auto ResourceManager::getTexture(std::string textureName) -> render::TextureId {
    ASSERT_MSG(textures.contains(textureName), textureName + " texture not in catch");
    return textures.find(textureName)->second;
}

void ResourceManager::addMesh(std::string meshName, add_mesh_func func) {
    ASSERT_MSG(!meshName.empty(), "textureName is null");
    ASSERT_MSG(func, "add_mesh_func is null");
    auto model = Model::createFromFile(meshName);
    auto meshId = func(model);
    ASSERT_MSG(!mesh.contains(meshName), meshName + " mesh in catch");
    mesh[meshName] = meshId;
}
void ResourceManager::addMesh(std::string meshName, const IMeshData& meshData, add_mesh_func func) {
    ASSERT_MSG(!meshName.empty(), "textureName is null");
    ASSERT_MSG(func, "add_mesh_func is null");
    auto meshId = func(meshData);
    ASSERT_MSG(!mesh.contains(meshName), meshName + " mesh in catch");
    mesh[meshName] = meshId;
}
auto ResourceManager::getMesh(std::string meshName) -> render::MeshId {
    ASSERT_MSG(mesh.contains(meshName), meshName + " texture in catch");
    return mesh.find(meshName)->second;
}

auto ResourceManager::getShaderCode(render::ShaderType type, const std::string& name)
    -> std::vector<std::uint32_t> {
    switch (type) {
        case render::ShaderType::Vertex:
            return read_shader(name + std::string("_vert"));
        case render::ShaderType::Fragment:
            return read_shader(name + std::string("_frag"));
        case render::ShaderType::Compute:
            return read_shader(name + std::string("_comp"));
        default:
            ASSERT_MSG(false, "Unsupported shader type");
            return {};
    }
}

}  // namespace graphics