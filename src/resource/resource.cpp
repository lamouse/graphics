#include "resource/resource.hpp"
#include "common/assert.hpp"
#include "resource/obj/model.hpp"
#include "resource/shader/shader.hpp"
#include <cstring>
#include <spdlog/spdlog.h>
namespace graphics {
void ResourceManager::addTexture(std::string textureName, add_texture_func func) {
    ASSERT_MSG(!textureName.empty(), "textureName is null");
    ASSERT_MSG(!textures.contains(textureName), textureName + " texture in catch");

    resource::image::Image texture(textureName);
    if (func) {
        textures[textureName] = func(texture);
    } else {
        textures[textureName] = graphic->uploadTexture(texture);
    }
}

void ResourceManager::addTexture(std::span<std::string> textureName, const std::string& name,
                                 add_texture_func func) {
    if (textureName.empty()) {
        return;
    }
    std::vector<resource::image::Image> images;
    int width{}, height{};
    std::vector<unsigned char> all_image;
    for (const auto& n : textureName) {
        resource::image::Image loaded_image{n};
        width = loaded_image.getWidth();
        height = loaded_image.getHeight();
        all_image.insert(all_image.end(), loaded_image.data().begin(), loaded_image.data().end());
    }

    resource::image::Image uploadImage{width, height, all_image,
                                       static_cast<std::uint8_t>(textureName.size())};
    if (func) {
        textures[name] = func(uploadImage);
    } else {
        textures[name] = graphic->uploadTexture(uploadImage);
    }
}

auto ResourceManager::getTexture(std::string textureName) const -> render::TextureId {
    if (textureName.empty()) {
        return {};
    }
    ASSERT_MSG(textures.contains(textureName), textureName + " texture not in catch");
    return textures.find(textureName)->second;
}

void ResourceManager::addMesh(std::string meshName, add_mesh_func func) {
    ASSERT_MSG(!meshName.empty(), "meshName is null");
    auto model_meshes = Model::createFromFile(meshName);
    for (const auto& model_mesh : model_meshes) {
        addMesh(meshName, model_mesh, std::move(func));
        auto meshId = getMesh(meshName);
        if (!model_mesh.only_vertex.empty()) {
            mesh_vertex[meshId] = std::make_unique<std::vector<::glm::vec3>>(model_mesh.only_vertex);
        }
        if (!model_mesh.save32_indices.empty()) {
            mesh_indics[meshId] = std::make_unique<std::vector<uint32_t>>(model_mesh.save32_indices);
        }
    }
}
void ResourceManager::addMesh(std::string meshName, const IMeshData& meshData, add_mesh_func func) {
    ASSERT_MSG(!meshName.empty(), "meshName is null");
    ASSERT_MSG(func || graphic, "add_mesh_func is null");
    if (mesh.contains(meshName)) {
        SPDLOG_WARN("meshName {} in cache", meshName);
        return;
    }

    render::MeshId meshId;
    if (func) {
        meshId = func(meshData);
    } else {
        meshId = graphic->uploadModel(meshData);
    }
    mesh[meshName] = meshId;
}
auto ResourceManager::getMesh(std::string meshName) const -> render::MeshId {
    if (meshName.empty()) {
        return {};
    }
    ASSERT_MSG(mesh.contains(meshName), meshName + " mesh in catch");
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

void ResourceManager::addGraphShader(
    const std::string& name,
    const std::function<std::uint64_t(std::span<const std::uint32_t>, render::ShaderType)>&
        upload_func) {
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

auto ResourceManager::getMeshVertex(const std::string& meshName) -> std::span<glm::vec3> {
    auto meshId = getMesh(meshName);

    if (meshId && mesh_vertex.contains(meshId)) {
        auto& data = mesh_vertex.find(meshId)->second;
        return std::span(data->data(), data->size());
    }
    return {};
}
auto ResourceManager::getMeshIndics(const std::string& meshName) -> std::span<uint32_t> {
    auto meshId = getMesh(meshName);
    if (meshId && mesh_indics.contains(meshId)) {
        auto& data = mesh_indics.find(meshId)->second;
        return std::span(data->data(), data->size());
    }
    return {};
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