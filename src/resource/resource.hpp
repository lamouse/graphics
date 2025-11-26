#pragma once
#include "render_core/types.hpp"
#include "render_core/graphic.hpp"
#include "resource/obj/mesh.hpp"
#include "resource/obj/model_mesh.hpp"
#include "common/common_funcs.hpp"
#include "resource/texture/image.hpp"
#include <unordered_map>
#include <string>
#include <functional>
#include <glm/glm.hpp>
#include <vector>

namespace graphics {

constexpr std::string_view DEFAULT_1X1_WRITE_TEXTURE{"__default_1x1_white_texture__"};
struct ShaderHash {
        std::uint64_t vertex;
        std::uint64_t fragment;
};

struct ModelConfig {
        std::string path;
        uint64_t hash;
        bool flip_uv{false};
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
        ~ResourceManager() = default;
        CLASS_NON_COPYABLE(ResourceManager);
        CLASS_DEFAULT_MOVEABLE(ResourceManager);
        auto addTexture(std::string_view textureName, const add_texture_func& func = nullptr)
            -> render::TextureId;
        /**
         * @brief 暂时针对cube map的6张图片
         *
         * @param textureNames
         * @param func
         */
        auto addCubeMapTexture(std::span<std::string> textureNames, const std::string& name,
                               const add_texture_func& func = nullptr) -> render::TextureId;

        auto addKtxCubeMap(std::string name) -> render::TextureId;
        auto addKtxTexture(std::string name) -> render::TextureId;
        [[nodiscard]] auto getTexture(std::string textureName) const -> render::TextureId;
        explicit ResourceManager(render::Graphic* graphic_);

        auto addModel(std::string_view path, add_mesh_func func = nullptr) -> render::MeshId;

        auto getModelConfig(std::string_view name) -> ModelConfig;
        auto addMesh(std::string meshName, const IMeshData&, add_mesh_func func = nullptr)
            -> render::MeshId;

        void addMeshVertex(render::MeshId meshId, const std::vector<glm::vec3>& vertexes,
                           const std::vector<uint32_t>& indics);

        [[nodiscard]] auto getModelSubMesh(render::MeshId id) const -> std::span<const SubMesh>;

        [[nodiscard]] auto getMesh(const std::string& name) const -> render::MeshId;
        auto addGraphShader(
            const std::string& name,
            const std::function<std::uint64_t(std::span<const std::uint32_t>, render::ShaderType)>&
                upload_func = nullptr) -> ShaderHash;
        void addComputeShader(
            const std::string& name,
            const std::function<std::uint64_t(std::span<const std::uint32_t>, render::ShaderType)>&
                upload_func = nullptr);

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

        auto getMeshVertex(render::MeshId id) -> std::span<glm::vec3>;
        auto getMeshIndics(render::MeshId id) -> std::span<uint32_t>;

    private:
        auto getShaderCode(render::ShaderType type, const std::string& name)
            -> std::vector<std::uint32_t>;
        void initializeDefaultTextures();
        std::unordered_map<std::string, render::TextureId> textures;
        std::unordered_map<std::string, render::MeshId> model_mesh_id_;
        std::unordered_map<render::MeshId, std::unique_ptr<std::vector<glm::vec3>>> mesh_vertex;
        std::unordered_map<render::MeshId, std::unique_ptr<std::vector<std::uint32_t>>> mesh_indics;
        std::unordered_map<std::string, std::uint64_t> compute_shader_hash;
        std::unordered_map<std::string, ShaderHash> graphic_shader_hash;
        std::unordered_map<std::string, std::uint64_t> model_file_hash;
        std::unordered_map<render::MeshId, std::unique_ptr<std::vector<SubMesh>>> model_sub_mesh;

        render::Graphic* graphic;
};
}  // namespace graphics
