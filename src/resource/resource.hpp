#pragma once
#include "render_core/types.hpp"
#include "render_core/shader_cache.hpp"
#include <unordered_map>
#include <string>
#include "common/common_funcs.hpp"
#include <functional>
#include "resource/texture/image.hpp"
#include "resource/obj/mesh.hpp"

namespace graphics {
using add_texture_func = std::function<render::TextureId(const resource::image::ITexture&)>;
using add_mesh_func = std::function<render::TextureId(const IMeshData&)>;
class ResourceManager {
    public:
        ResourceManager() = default;
        ~ResourceManager() = default;
        CLASS_NON_COPYABLE(ResourceManager);
        CLASS_DEFAULT_MOVEABLE(ResourceManager);
        void addTexture(std::string textureName, add_texture_func func);
        auto getTexture(std::string textureName) -> render::TextureId;

        void addMesh(std::string meshName, add_mesh_func func);
        auto getMesh(std::string meshName) -> render::MeshId;
        auto getShaderCode(render::ShaderType type)-> std::vector<std::uint32_t>;
    private:
        std::unordered_map<std::string, render::TextureId> textures;
        std::unordered_map<std::string, render::MeshId> mesh;
};
}  // namespace graphics
