#include "resource/resource.hpp"
#include "common/assert.hpp"
#include "resource/obj/model.hpp"
#include "resource/shader/shader.hpp"
namespace graphics {
void ResourceManager::addTexture(std::string textureName, add_texture_func func) {
    ASSERT_MSG(!textureName.empty(), "textureName is null");
    ASSERT_MSG(!textures.contains(textureName), textureName + " texture in catch");

    resource::image::Image texture(textureName);
    if(func){
        textures[textureName] = func(texture);
    }else{
        textures[textureName] = graphic->uploadTexture(texture);
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
    auto model = Model::createFromFile(meshName);
    addMesh(meshName, model, std::move(func));
}
void ResourceManager::addMesh(std::string meshName, const IMeshData& meshData, add_mesh_func func) {
    ASSERT_MSG(!meshName.empty(), "meshName is null");
    ASSERT_MSG(func || graphic, "add_mesh_func is null");
    render::MeshId meshId;
    if (func) {
        meshId = func(meshData);
    } else {
        meshId = graphic->uploadModel(meshData);
    }
    ASSERT_MSG(!mesh.contains(meshName), meshName + " mesh in catch");
    mesh[meshName] = meshId;
}
auto ResourceManager::getMesh(std::string meshName) const -> render::MeshId {
    if (meshName.empty()) {
        return {};
    }
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
// 显式实例化模板成员函数
template ShaderHash ResourceManager::getShaderHash<ShaderHash>(const std::string& name) const;
template std::uint64_t ResourceManager::getShaderHash<std::uint64_t>(const std::string& name) const;
}  // namespace graphics