#include "resource/resource.hpp"
#include "common/assert.hpp"
namespace graphics {
void ResourceManager::addTexture(std::string textureName, add_texture_func func) {
    ASSERT_MSG(!textureName.empty(), "textureName is null");
    ASSERT_MSG(func, "add_texture_func is null");
    resource::image::Image texture(textureName);
    auto textureId = func(texture);
    ASSERT_MSG(!textures.contains(textureName), textureName + " texture in catch");
    textures[textureName] = textureId;
}

auto ResourceManager::getTexture(std::string textureName) -> render::TextureId{
    ASSERT_MSG(textures.contains(textureName), textureName + " texture in catch");
    return textures.find(textureName)->second;
}
}  // namespace graphics