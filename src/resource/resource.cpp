#include "resource/resource.hpp"
#include "common/assert.hpp"
#include "resource/obj/model_mesh.hpp"
#include "resource/shader/shader.hpp"
#include "resource/texture/ktx_image.hpp"
#include "render_core/types.hpp"
#include "model_config.hpp"
#include "image_config.hpp"
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <utility>

namespace graphics {
auto ResourceManager::addTexture(std::string_view textureName, const add_texture_func& func)
    -> render::TextureId {
    ASSERT_MSG(!textureName.empty(), "textureName is null");

    const auto [pair, is_new]{textures.try_emplace(std::string(textureName))};
    if (!is_new) {
        return pair->second;
    }
    resource::image::Image texture(textureName);
    render::TextureId id;
    if (func) {
        id = func(texture);
    } else {
        id = graphic->uploadTexture(texture);
    }
    pair->second = id;
    return id;
}

auto ResourceManager::addKtxCubeMap(std::string name) -> render::TextureId {
    ASSERT_MSG(!name.empty(), "textureName is null");
    const auto [pair, is_new]{textures.try_emplace(name)};
    if (!is_new) {
        return pair->second;
    }
    resource::image::KtxImage image(texture::CUBE_MAP_PATH + name);
    auto* texture = image.getKtxTexture();
    auto id = graphic->uploadTexture(texture);
    pair->second = id;
    return id;
}

auto ResourceManager::addKtxTexture(std::string name) -> render::TextureId {
    ASSERT_MSG(!name.empty(), "textureName is null");
    const auto [pair, is_new]{textures.try_emplace(name)};
    if (!is_new) {
        return pair->second;
    }
    std::filesystem::path file{name};
    file.replace_extension("ktx2");
    resource::image::KtxImage image(texture::TEXTURE_ROOT_PATH + file.string());
    auto* texture = image.getKtxTexture();
    auto id = graphic->uploadTexture(texture);
    pair->second = id;
    return id;
}

auto ResourceManager::addCubeMapTexture(std::span<std::string> textureNames,
                                        const std::string& name, const add_texture_func& func)
    -> render::TextureId {
    if (textureNames.empty()) {
        return {};
    }
    std::vector<resource::image::Image> images;
    int width{}, height{};
    std::vector<unsigned char> all_image;
    for (const auto& n : textureNames) {
        resource::image::Image loaded_image{n};
        width = loaded_image.getWidth();
        height = loaded_image.getHeight();
        all_image.insert(all_image.end(), loaded_image.data().begin(), loaded_image.data().end());
    }

    resource::image::Image uploadImage{width, height, all_image,
                                       static_cast<std::uint8_t>(textureNames.size())};
    render::TextureId id;
    if (func) {
        id = func(uploadImage);
    } else {
        id = graphic->uploadTexture(uploadImage);
    }
    textures[name] = id;
    return id;
}

auto ResourceManager::getTexture(std::string textureName) const -> render::TextureId {
    if (textureName.empty()) {
        return {};
    }
    ASSERT_MSG(textures.contains(textureName), textureName + " texture not in catch");
    return textures.find(textureName)->second;
}

auto ResourceManager::addModel(std::string_view model_path, add_mesh_func func) -> render::MeshId {
    ASSERT_MSG(!model_path.empty(), "meshName is null");
    namespace fs = std::filesystem;
    fs::path file_path(std::string(model::MODEL_ROOT_PATH) + std::string(model_path));
    std::string model_file_path{model_path};
    bool flip_uv{false};
    if (fs::is_directory(file_path)) {
        using json = nlohmann::json;
        std::ifstream f(file_path.string() + "/config.json");
        json j;
        f >> j;
        model_file_path = model_file_path + "/" + j["name"].get<std::string>();

        flip_uv = j.contains("need_flip_uv") && j["need_flip_uv"].get<bool>();
    }
    auto model_ = Model::createFromFile(model::MODEL_ROOT_PATH + model_file_path,
                                        model_file_hash[model_file_path], flip_uv);
    auto mesh_id = addMesh(std::string(model_path), model_, std::move(func));
    mesh_vertex[mesh_id] = std::make_unique<std::vector<::glm::vec3>>(model_.only_vertex);
    mesh_indics[mesh_id] = std::make_unique<std::vector<uint32_t>>(model_.indices_);
    model_sub_mesh[mesh_id] = std::make_unique<std::vector<SubMesh>>(model_.subMeshes);
    return mesh_id;
}

auto ResourceManager::getModelConfig(std::string_view name) -> ModelConfig {
    ModelConfig config;
    config.hash = model_file_hash[std::string(name)];
    config.path = std::string(model::MODEL_ROOT_PATH) + std::string(name);
    namespace fs = std::filesystem;
    fs::path file_path(std::string(model::MODEL_ROOT_PATH) + std::string(name));
    if (fs::is_directory(file_path)) {
        using json = nlohmann::json;
        std::ifstream f(file_path.string() + "/config.json");
        json j;
        f >> j;
        config.path = file_path.string() + "/" + j["name"].get<std::string>();
        config.flip_uv = j.contains("need_flip_uv") && j["need_flip_uv"].get<bool>();
        config.hash = model_file_hash[std::string(name) + "/" + j["name"].get<std::string>()];
    }

    return config;
}

void ResourceManager::addMeshVertex(render::MeshId meshId, const std::vector<glm::vec3>& vertexes,
                                    const std::vector<uint32_t>& indics) {
    if (vertexes.empty()) {
        return;
    }
    mesh_vertex[meshId] = std::make_unique<std::vector<::glm::vec3>>(vertexes);
    mesh_indics[meshId] = std::make_unique<std::vector<uint32_t>>(indics);
}

auto ResourceManager::addMesh(std::string meshName, const IMeshData& meshData, add_mesh_func func)
    -> render::MeshId {
    ASSERT_MSG(!meshName.empty(), "meshName is null");
    ASSERT_MSG(func || graphic, "add_mesh_func is null");
    render::MeshId meshId;
    if (func) {
        meshId = func(meshData);
    } else {
        meshId = graphic->uploadModel(meshData);
    }
    model_mesh_id_[meshName] = meshId;
    return meshId;
}
auto ResourceManager::getMesh(const std::string& name) const -> render::MeshId {
    if (name.empty()) {
        return {};
    }
    ASSERT_MSG(model_mesh_id_.contains(name), meshName + " mesh in catch");
    return model_mesh_id_.find(name)->second;
}

[[nodiscard]] auto ResourceManager::getModelSubMesh(render::MeshId id) const
    -> std::span<const SubMesh> {
    if (!model_sub_mesh.contains(id)) {
        return {};
    }
    const auto& sub_mesh = model_sub_mesh.find(id)->second;
    return std::span<const SubMesh>(sub_mesh->data(), sub_mesh->size());
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

template <render::ShaderType type>
auto ResourceManager::getShaderHash(const std::string& name) const -> std::uint64_t {
    if constexpr (type == render::ShaderType::Vertex) {
        if (graphic_shader_hash.contains(name)) {
            return graphic_shader_hash.find(name)->second.vertex;
        }
    }

    if constexpr (type == render::ShaderType::Fragment) {
        if (graphic_shader_hash.contains(name)) {
            return graphic_shader_hash.find(name)->second.fragment;
        }
    }

    if constexpr (type == render::ShaderType::Compute) {
        if (compute_shader_hash.contains(name)) {
            return compute_shader_hash.find(name)->second;
        }
    }
    ASSERT_MSG(false, "get shader not found");
    return 0;
}

template <typename T>
    requires(IsUint64<T> || IsShaderHashStruct<T>)
[[nodiscard]] auto ResourceManager::getShaderHash(const std::string& name) const -> T {
    if constexpr (std::is_same_v<T, std::uint64_t>) {
        if (compute_shader_hash.contains(name)) {
            return compute_shader_hash.find(name)->second;
        }
    }
    if constexpr (std::is_same_v<T, ShaderHash>) {
        if (graphic_shader_hash.contains(name)) {
            return graphic_shader_hash.find(name)->second;
        }
    }
    ASSERT_MSG(false, "get shader not found or type error");

    return T{};
}

auto ResourceManager::addGraphShader(
    const std::string& name,
    const std::function<std::uint64_t(std::span<const std::uint32_t>, render::ShaderType)>&
        upload_func) -> ShaderHash {
    auto vertex_shader_code = getShaderCode(render::ShaderType::Vertex, name);
    auto fragment_shader_code = getShaderCode(render::ShaderType::Fragment, name);

    ShaderHash hash{};
    if (upload_func) {
        hash.vertex = upload_func(vertex_shader_code, render::ShaderType::Vertex);
        hash.fragment = upload_func(fragment_shader_code, render::ShaderType::Fragment);
    } else {
        hash.vertex = graphic->addShader(vertex_shader_code, render::ShaderType::Vertex);
        hash.fragment = graphic->addShader(fragment_shader_code, render::ShaderType::Fragment);
    }
    graphic_shader_hash[name] = hash;
    return hash;
}
void ResourceManager::addComputeShader(
    const std::string& name,
    const std::function<std::uint64_t(std::span<const std::uint32_t>, render::ShaderType)>&
        upload_func) {
    auto shader_code = getShaderCode(render::ShaderType::Compute, name);
    std::uint64_t hash{};
    if (upload_func) {
        hash = upload_func(shader_code, render::ShaderType::Compute);
    } else {
        hash = graphic->addShader(shader_code, render::ShaderType::Compute);
    }
    compute_shader_hash[name] = hash;
}

auto ResourceManager::getMeshVertex(render::MeshId id) -> std::span<glm::vec3> {
    if (mesh_vertex.contains(id)) {
        auto& data = mesh_vertex.find(id)->second;
        return std::span(data->data(), data->size());
    }
    return {};
}
auto ResourceManager::getMeshIndics(render::MeshId id) -> std::span<uint32_t> {
    if (mesh_indics.contains(id)) {
        auto& data = mesh_indics.find(id)->second;
        return std::span(data->data(), data->size());
    }
    return {};
}

ResourceManager::ResourceManager(render::Graphic* graphic_) : graphic(graphic_) {
    using json = nlohmann::json;
    std::ifstream f(model::MODEL_HASH_PATH);
    json j;
    f >> j;
    for (const auto& m : j["assets"]) {
        model_file_hash[m["path"]] = m["hash"];
    }

    initializeDefaultTextures();
}

void ResourceManager::initializeDefaultTextures() {
    std::array<unsigned char, 4> withe{255, 255, 255, 255};
    resource::image::Image white_texture(1, 1, withe, 1);
    if (graphic) {
        auto white_texture_id = graphic->uploadTexture(white_texture);
        textures[std::string(DEFAULT_1X1_WRITE_TEXTURE)] = white_texture_id;
    }
}

// 显式实例化模板成员函数
template ShaderHash ResourceManager::getShaderHash<ShaderHash>(const std::string& name) const;
template std::uint64_t ResourceManager::getShaderHash<std::uint64_t>(const std::string& name) const;
}  // namespace graphics