#include "model.hpp"

#include "common/file.hpp"
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <assimp/Importer.hpp>
#include <cassert>
#include <format>
#include <fstream>

namespace {
constexpr const char* model_cache_path = "data/cache/mesh/";
constexpr const char* model_cache_extend = ".mesh";
template <typename T>
auto as_bytes(std::span<const T> s) -> std::span<const std::byte> {
    // 将 T* 指针 reinterpret_cast 为 const std::byte* 指针
    const auto* ptr = reinterpret_cast<const std::byte*>(s.data());
    // 第二个参数是 span 的“元素”数量，即字节数
    return std::span<const std::byte>(ptr, s.size() * sizeof(T));
}

void saveToCache(const std::string& cachePath, const std::vector<graphics::Model>& models,
                 uint64_t objFileHash) {
    common::create_dir(model_cache_path);
    std::ofstream file(cachePath, std::ios::binary);
    if (!file) {
        return;
    }

    // Header
    graphics::ModelCacheHeader header;
    header.objFileHash = objFileHash;
    header.modelCount = static_cast<uint32_t>(models.size());
    file.write(reinterpret_cast<const char*>(&header), sizeof(header));

    // Gather all data & build descriptors
    std::vector<graphics::Model::Vertex> allVertices;
    std::vector<uint32_t> allIndices;
    std::vector<glm::vec3> allOnlyVertices;

    std::vector<graphics::ModelDesc> descs;
    for (const auto& model : models) {
        graphics::ModelDesc desc;
        desc.vertexCount = model.vertices_.size();
        desc.indexCount = model.indices_.size();
        desc.onlyVertexCount = model.only_vertex.size();

        desc.vertexOffset = allVertices.size();
        desc.indexOffset = allIndices.size();
        desc.onlyVertexOffset = allOnlyVertices.size();

        // Append data
        allVertices.insert(allVertices.end(), model.vertices_.begin(), model.vertices_.end());
        allIndices.insert(allIndices.end(), model.indices_.begin(), model.indices_.end());
        allOnlyVertices.insert(allOnlyVertices.end(), model.only_vertex.begin(),
                               model.only_vertex.end());

        descs.push_back(desc);
    }

    // Write ModelDesc array
    file.write(reinterpret_cast<const char*>(descs.data()),
               sizeof(graphics::ModelDesc) * descs.size());

    // Write data blocks (raw binary)
    file.write(reinterpret_cast<const char*>(allVertices.data()),
               sizeof(graphics::Model::Vertex) * allVertices.size());
    file.write(reinterpret_cast<const char*>(allIndices.data()),
               sizeof(uint32_t) * allIndices.size());
    file.write(reinterpret_cast<const char*>(allOnlyVertices.data()),
               sizeof(glm::vec3) * allOnlyVertices.size());

    // 🔥 NEW: Write all MeshMaterial entries
    for (const auto& model : models) {
        model.material.serialize(file);
    }
}

auto loadModelWithCache(std::uint64_t file_hash) -> std::optional<std::vector<graphics::Model>> {
    auto cachePath = std::string(model_cache_path) + std::to_string(file_hash) + model_cache_extend;
    std::ifstream file(cachePath, std::ios::binary);
    if (!file) {
        return std::nullopt;
    }
    graphics::ModelCacheHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(header));
    if (header.magic != graphics::ModelCacheHeader::MAGIC || header.version != graphics::MESH_CACHE_VERSION) {
        return std::nullopt;
    }
    if (header.objFileHash != file_hash) {
        return std::nullopt;  // outdated
    }

    std::vector<graphics::ModelDesc> descs(header.modelCount);
    file.read(reinterpret_cast<char*>(descs.data()),
              sizeof(graphics::ModelDesc) * header.modelCount);

    // Pre-read all data blocks
    size_t totalVertices = 0, totalIndices = 0, totalOnly = 0;
    for (const auto& d : descs) {
        totalVertices += d.vertexCount;
        totalIndices += d.indexCount;
        totalOnly += d.onlyVertexCount;
    }

    std::vector<graphics::Model::Vertex> allVertices(totalVertices);
    std::vector<uint32_t> allIndices(totalIndices);
    std::vector<glm::vec3> allOnlyVertices(totalOnly);

    file.read(reinterpret_cast<char*>(allVertices.data()),
              sizeof(graphics::Model::Vertex) * totalVertices);
    file.read(reinterpret_cast<char*>(allIndices.data()), sizeof(uint32_t) * totalIndices);
    file.read(reinterpret_cast<char*>(allOnlyVertices.data()), sizeof(glm::vec3) * totalOnly);

    // 🔥 NEW: Read all materials
    std::vector<graphics::MeshMaterial> materials;
    materials.reserve(header.modelCount);
    for (uint32_t i = 0; i < header.modelCount; ++i) {
        graphics::MeshMaterial mat;
        if (!mat.deserialize(file)) {
            return std::nullopt;  // corrupted
        }
        materials.push_back(std::move(mat));
    }

    // Reconstruct models
    std::vector<graphics::Model> models;
    models.reserve(header.modelCount);
    for (size_t i = 0; i < header.modelCount; ++i) {
        const auto& d = descs[i];
        const auto& mat = materials[i];  // 🔥 use cached material
        std::vector<graphics::Model::Vertex> verts(
            allVertices.begin() + d.vertexOffset,
            allVertices.begin() + d.vertexOffset + d.vertexCount);
        std::vector<uint32_t> indices(allIndices.begin() + d.indexOffset,
                                      allIndices.begin() + d.indexOffset + d.indexCount);
        std::vector<glm::vec3> onlyVerts(
            allOnlyVertices.begin() + d.onlyVertexOffset,
            allOnlyVertices.begin() + d.onlyVertexOffset + d.onlyVertexCount);

        models.emplace_back(std::move(verts), std::move(indices), std::move(onlyVerts), mat);
    }

    return models;
}

}  // namespace

namespace graphics {

Model::Model(const ::std::vector<Vertex>& vertices, const ::std::vector<uint32_t>& indices,
             const std::vector<::glm::vec3>& only_vertex_, MeshMaterial material_)
    : only_vertex(only_vertex_),
      indices_(indices),
      vertices_(vertices),
      material(std::move(material_)),
      vertexCount(static_cast<uint32_t>(vertices.size())),
      indicesSize(static_cast<uint32_t>(indices.size())) {
    assert(vertexCount >= 3 && "Vertex count must be at least 3");
}

auto Model::createFromFile(const ::std::string& path, std::uint64_t obj_hash)
    -> std::vector<Model> {
    if (obj_hash == 0) {
        auto file_hash = common::file_hash(path);
        obj_hash = file_hash ? file_hash.value() : 0;
    }
    if (!obj_hash) {
        return {};
    }
    if (auto meshes = loadModelWithCache(obj_hash)) {
        return std::move(meshes.value());
    }

    Assimp::Importer importer;
    constexpr auto ASSIMP_LOAD_FLAGS =
        aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices;
    // NOLINTNEXTLINE
    const aiScene* scene = importer.ReadFile(path, ASSIMP_LOAD_FLAGS);
    if (!scene) {
        throw std::runtime_error(
            std::format("Failed to load model: {}", importer.GetErrorString()));
    }
    std::vector<Model> mesh_models;
    for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[i];

        std::vector<Model::Vertex> vertices;
        std::vector<uint32_t> indices;
        std::vector<glm::vec3> position;
        vertices.reserve(mesh->mNumVertices);
        position.reserve(mesh->mNumVertices);

        for (unsigned int j = 0; j < mesh->mNumVertices; j++) {
            aiVector3D vertex = mesh->mVertices[j];
            Model::Vertex model_vertex{};
            model_vertex.position = {vertex.x, vertex.y, vertex.z};
            position.push_back(model_vertex.position);
            if (mesh->HasTextureCoords(0)) {
                const auto* textureCoords = mesh->mTextureCoords[0];
                model_vertex.texCoord = {textureCoords[j].x, textureCoords[j].y};
            } else {
                model_vertex.texCoord = {.0f, .0f};
            }

            if (mesh->HasNormals()) {
                model_vertex.normal = {mesh->mNormals[j].x, mesh->mNormals[j].y,
                                       mesh->mNormals[j].z};
            } else {
                model_vertex.normal = {0.0f, 0.0f, 0.0f};
            }
            if (mesh->HasVertexColors(0)) {
                const auto* vertex_color = mesh->mColors[0];
                model_vertex.color = {vertex_color[j].r, vertex_color[j].g, vertex_color[j].b};
            } else {
                model_vertex.color = {1.0f, 1.0f, 1.0f};
            }
            vertices.push_back(model_vertex);
        }

        indices.reserve(static_cast<std::size_t>(mesh->mNumFaces * 3));
        for (u32 f = 0; f < mesh->mNumFaces; f++) {
            const auto face = mesh->mFaces[f];
            ASSERT_MSG(face.mNumIndices == 3, "face indices not equal 3");
            for (unsigned int idx = 0; idx < 3; ++idx) {
                auto globalIndex = static_cast<uint32_t>(face.mIndices[idx]);
                indices.push_back(globalIndex);
            }
        }
        MeshMaterial meshMaterial_{};
        if (scene->HasMaterials()) {
            aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
            if (material) {
                aiColor3D ka, kd, ks, ke;
                material->Get(AI_MATKEY_COLOR_AMBIENT, ka);
                meshMaterial_.ambientColor = {ka.r, ka.g, ka.b};
                material->Get(AI_MATKEY_COLOR_DIFFUSE, kd);
                meshMaterial_.diffuseColor = {kd.r, kd.g, kd.b};
                material->Get(AI_MATKEY_COLOR_SPECULAR, ks);
                meshMaterial_.specularColor = {ks.r, ks.g, ks.b};
                material->Get(AI_MATKEY_COLOR_EMISSIVE, ke);
                meshMaterial_.emissiveColor = {ke.r, ke.g, ke.b};

                material->Get(AI_MATKEY_SHININESS, meshMaterial_.shininess);
                material->Get(AI_MATKEY_OPACITY, meshMaterial_.opacity);
                material->Get(AI_MATKEY_REFRACTI,
                              meshMaterial_.ior);  // 注意：有些导出器用 AI_MATKEY_REFRACTI_VACUUM

                aiString texture_path;
                if (material->GetTexture(aiTextureType_AMBIENT, 0, &texture_path) == AI_SUCCESS) {
                    meshMaterial_.ambientTexture = texture_path.C_Str();
                }
                if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texture_path) == AI_SUCCESS) {
                    meshMaterial_.diffuseTexture = texture_path.C_Str();
                }
                if (material->GetTexture(aiTextureType_SPECULAR, 0, &texture_path) == AI_SUCCESS) {
                    meshMaterial_.specularTexture = texture_path.C_Str();
                }
                if (material->GetTexture(aiTextureType_NORMALS, 0, &texture_path) == AI_SUCCESS) {
                    meshMaterial_.normalTexture = texture_path.C_Str();
                }
                if (material->GetTexture(aiTextureType_EMISSIVE, 0, &texture_path) == AI_SUCCESS) {
                    meshMaterial_.emissiveTexture = texture_path.C_Str();
                }
            }
        }

        mesh_models.emplace_back(vertices, indices, position, meshMaterial_);
    }
    std::string cache_path = model_cache_path + std::to_string(obj_hash) + model_cache_extend;
    saveToCache(cache_path, mesh_models, obj_hash);

    return mesh_models;
}

// 返回顶点坐标（仅 position），展平为 float 数组
[[nodiscard]] auto Model::getMesh() const -> std::span<const float> {
    return std::span<const float>(reinterpret_cast<const float*>(vertices_.data()),
                                  vertices_.size() * sizeof(Model::Vertex) / sizeof(float));
}

auto Model::getIndices() const -> std::span<const std::byte> {
    return as_bytes(std::span(indices_));
}

}  // namespace graphics
