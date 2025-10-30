#pragma once
#include "render_core/types.hpp"
#include "render_core/graphic.hpp"
#include <unordered_map>
#include <string>
#include "common/common_funcs.hpp"
#include <functional>
#include "resource/texture/image.hpp"
#include "resource/obj/mesh.hpp"

namespace graphics {
struct ShaderHash {
        std::uint64_t vertex;
        std::uint64_t fragment;
};
// Concept：匹配 ShaderHash 结构体
template <typename T>
concept IsShaderHashStruct = std::same_as<T, ShaderHash>;

// Concept：匹配 uint64_t
template <typename T>
concept IsUint64 = std::same_as<T, std::uint64_t>;

using add_texture_func = std::function<render::TextureId(const resource::image::ITexture&)>;
using add_mesh_func = std::function<render::TextureId(const IMeshData&)>;
class ResourceManager {
    public:
        ResourceManager() = default;
        ~ResourceManager() = default;
        CLASS_NON_COPYABLE(ResourceManager);
        CLASS_DEFAULT_MOVEABLE(ResourceManager);
        void addTexture(std::string textureName, add_texture_func func);
        [[nodiscard]] auto getTexture(std::string textureName) const -> render::TextureId;
        void setGraphic(render::Graphic* graphic_) { graphic = graphic_; }

        void addMesh(std::string meshName, add_mesh_func func = nullptr);
        void addMesh(std::string meshName, const IMeshData&, add_mesh_func func = nullptr);

        [[nodiscard]] auto getMesh(std::string meshName) const -> render::MeshId;
        void addGraphShader(const std::string& name,
                            const std::function<std::uint64_t(std::span<const std::uint32_t>,
                                                              render::ShaderType)>& upload_func = nullptr);
        void addComputeShader(const std::string& name,
                              const std::function<std::uint64_t(std::span<const std::uint32_t>,
                                                                render::ShaderType)>& upload_func = nullptr);

        template <render::ShaderType type>
        [[nodiscard]] auto getShaderHash(const std::string& name) const -> std::uint64_t;

        /**
         * @brief std::uint64返回computer shader hash， ShaderHash return graphic hash
         *
         * @tparam T
         */
        template <typename T>
            requires(IsUint64<T> || IsShaderHashStruct<T>)
        [[nodiscard]] auto getShaderHash(const std::string& name) const -> T;

    private:
        auto getShaderCode(render::ShaderType type, const std::string& name)
            -> std::vector<std::uint32_t>;
        std::unordered_map<std::string, render::TextureId> textures;
        std::unordered_map<std::string, render::MeshId> mesh;
        std::unordered_map<std::string, std::uint64_t> compute_shader_hash;
        std::unordered_map<std::string, ShaderHash> graphic_shader_hash;

        render::Graphic* graphic;
};
}  // namespace graphics
