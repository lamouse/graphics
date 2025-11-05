#include "resource/resource.hpp"
#include "common/assert.hpp"
#include "resource/obj/model.hpp"
#include "resource/shader/shader.hpp"
#include "render_core/types.hpp"
#include <spdlog/spdlog.h>
namespace graphics {
auto ResourceManager::addTexture(std::string textureName, const add_texture_func& func)
    -> render::TextureId {
    ASSERT_MSG(!textureName.empty(), "textureName is null");
    ASSERT_MSG(!textures.contains(textureName), textureName + " texture in catch");

    resource::image::Image texture(textureName);
    render::TextureId id;
    if (func) {
        id = func(texture);
    } else {
        id = graphic->uploadTexture(texture);
    }
    textures[textureName] = id;
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

auto ResourceManager::addModel(std::string modelName, add_mesh_func func)
    -> std::vector<render::MeshId> {
    ASSERT_MSG(!modelName.empty(), "meshName is null");
    auto model_meshes = Model::createFromFile(modelName);
    std::vector<render::MeshId> mesh_ids;
    model_meshes.reserve(model_meshes.size());
    for (const auto& model_mesh : model_meshes) {
        auto id = addMesh(modelName, model_mesh, std::move(func));
        if (!model_mesh.only_vertex.empty()) {
            mesh_vertex[id] = std::make_unique<std::vector<::glm::vec3>>(model_mesh.only_vertex);
        }
        if (!model_mesh.indices_.empty()) {
            mesh_indics[id] = std::make_unique<std::vector<uint32_t>>(model_mesh.indices_);
        }
        mesh_ids.push_back(id);
    }
    return mesh_ids;
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
    if (model_mesh_ids_.contains(meshName)) {
        model_mesh_ids_.find(meshName)->second.push_back(meshId);
    } else {
        model_mesh_ids_[meshName] = std::vector<render::MeshId>{meshId};
    }
    return meshId;
}
auto ResourceManager::getMesh(const std::string& name) const -> std::span<const render::MeshId> {
    if (name.empty()) {
        return {};
    }
    ASSERT_MSG(model_mesh_ids_.contains(name), meshName + " mesh in catch");
    const auto& mesh_ids = model_mesh_ids_.find(name)->second;
    return std::span(mesh_ids.data(), mesh_ids.size());
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

// 显式实例化模板成员函数
template ShaderHash ResourceManager::getShaderHash<ShaderHash>(const std::string& name) const;
template std::uint64_t ResourceManager::getShaderHash<std::uint64_t>(const std::string& name) const;
}  // namespace graphics