#include "resource/resource.hpp"
#include "common/assert.hpp"
#include "resource/obj/model.hpp"
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
    ASSERT_MSG(func, "add_texture_func is null");
    auto model = Model::createFromFile(meshName);
    auto textureId = func(model);
    ASSERT_MSG(!mesh.contains(meshName), meshName + " texture in catch");
    mesh[meshName] = textureId;
}
auto ResourceManager::getMesh(std::string meshName) -> render::MeshId {
    ASSERT_MSG(mesh.contains(meshName), meshName + " texture in catch");
    return mesh.find(meshName)->second;
}

}  // namespace graphics