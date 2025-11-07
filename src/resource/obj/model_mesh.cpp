#include "model_mesh.hpp"

#include "common/file.hpp"
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <math.h>
#include <math.h>

#include <assimp/Importer.hpp>
#include <cassert>
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

void saveModelToCache(std::uint64_t file_hash, const graphics::Model& model) {
    common::create_dir(model_cache_path);
    auto cachePath = std::string(model_cache_path) + std::to_string(file_hash) + model_cache_extend;
    std::ofstream file(cachePath, std::ios::binary);
    if (!file) {
        return;
    }

    graphics::ModelCacheHeader header{};
    // ✅ 显式初始化所有字段（推荐用 {} 零初始化 + 赋值）
    header.magic = graphics::ModelCacheHeader::MAGIC;
    header.version = graphics::MESH_CACHE_VERSION;
    header.objFileHash = file_hash;
    header.subMeshCount = static_cast<uint32_t>(model.subMeshes.size());

    file.write(reinterpret_cast<const char*>(&header), sizeof(header));

    // 写入数量
    uint64_t vcount = model.vertices_.size();
    uint64_t icount = model.indices_.size();
    uint64_t ocount = model.only_vertex.size();
    file.write(reinterpret_cast<const char*>(&vcount), sizeof(vcount));
    file.write(reinterpret_cast<const char*>(&icount), sizeof(icount));
    file.write(reinterpret_cast<const char*>(&ocount), sizeof(ocount));

    // 写入数据
    if (vcount > 0) {
        file.write(reinterpret_cast<const char*>(model.vertices_.data()),
                   sizeof(graphics::Model::Vertex) * vcount);
    }
    if (icount > 0) {
        file.write(reinterpret_cast<const char*>(model.indices_.data()), sizeof(uint32_t) * icount);
    }
    if (ocount > 0) {
        file.write(reinterpret_cast<const char*>(model.only_vertex.data()),
                   sizeof(glm::vec3) * ocount);
    }

    // 写入 submeshes
    for (const auto& sub : model.subMeshes) {
        file.write(reinterpret_cast<const char*>(&sub.indexOffset), sizeof(sub.indexOffset));
        file.write(reinterpret_cast<const char*>(&sub.indexCount), sizeof(sub.indexCount));
        sub.material.serialize(file);
    }
}

auto getTexturePath(aiMaterial* mat, aiTextureType type) -> std::vector<std::string> {
    std::vector<std::string> paths;
    for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
        aiString str;
        if (mat->GetTexture(type, i, &str) == AI_SUCCESS) {
            paths.emplace_back(str.C_Str());
        }
    }
    return paths;
}

auto loadModelWithCache(std::uint64_t file_hash) -> std::optional<graphics::Model> {
    auto cachePath = std::string(model_cache_path) + std::to_string(file_hash) + model_cache_extend;
    std::ifstream file(cachePath, std::ios::binary);
    if (!file) {
        return std::nullopt;
    }

    graphics::ModelCacheHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(header));
    if (header.magic != graphics::ModelCacheHeader::MAGIC ||
        header.version != graphics::MESH_CACHE_VERSION || header.objFileHash != file_hash) {
        return std::nullopt;
    }

    // 读取全局顶点/索引数量
    uint64_t vertexCount = 0, indexCount = 0, onlyVertexCount = 0;
    file.read(reinterpret_cast<char*>(&vertexCount), sizeof(vertexCount));
    file.read(reinterpret_cast<char*>(&indexCount), sizeof(indexCount));
    file.read(reinterpret_cast<char*>(&onlyVertexCount), sizeof(onlyVertexCount));

    // 读取数据
    std::vector<graphics::Model::Vertex> vertices(vertexCount);
    std::vector<uint32_t> indices(indexCount);
    std::vector<glm::vec3> only_vertices(onlyVertexCount);

    file.read(reinterpret_cast<char*>(vertices.data()),
              sizeof(graphics::Model::Vertex) * vertexCount);
    file.read(reinterpret_cast<char*>(indices.data()), sizeof(uint32_t) * indexCount);
    file.read(reinterpret_cast<char*>(only_vertices.data()), sizeof(glm::vec3) * onlyVertexCount);

    // 读取所有 SubMesh
    std::vector<graphics::SubMesh> subMeshes;
    subMeshes.reserve(header.subMeshCount);
    for (uint32_t i = 0; i < header.subMeshCount; ++i) {
        graphics::SubMesh sub;
        file.read(reinterpret_cast<char*>(&sub.indexOffset), sizeof(sub.indexOffset));
        file.read(reinterpret_cast<char*>(&sub.indexCount), sizeof(sub.indexCount));
        if (!sub.material.deserialize(file)) {
            return std::nullopt;
        }
        subMeshes.push_back(std::move(sub));
    }

    // 构建 Model
    graphics::Model model;
    model.vertices_ = std::move(vertices);
    model.indices_ = std::move(indices);
    model.only_vertex = std::move(only_vertices);
    model.subMeshes = std::move(subMeshes);

    // 返回 vector（兼容你现有接口）
    return std::move(model);
}

auto loadModelFromAssimpScene(const aiScene* scene) -> graphics::Model {
    graphics::Model model;

    uint32_t vertexOffset = 0;  // 当前 mesh 的顶点在全局 vertices_ 中的起始索引
    uint32_t indexOffset = 0;   // 当前 mesh 的索引在全局 indices_ 中的起始位置

    for (unsigned int i = 0; i < scene->mNumMeshes; ++i) {
        aiMesh* mesh = scene->mMeshes[i];
        if (!mesh) {
            continue;
        }

        // === 1. 加载顶点 ===
        for (unsigned int j = 0; j < mesh->mNumVertices; ++j) {
            graphics::Model::Vertex v{};

            // Position
            const aiVector3D& pos = mesh->mVertices[j];
            v.position = {pos.x, pos.y, pos.z};
            model.only_vertex.push_back(v.position);  // 你额外需要的

            // TexCoord
            if (mesh->HasTextureCoords(0)) {
                const aiVector3D& tex = mesh->mTextureCoords[0][j];
                v.texCoord = {tex.x, tex.y};
            } else {
                v.texCoord = {0.0f, 0.0f};
            }

            // Normal
            if (mesh->HasNormals()) {
                const aiVector3D& n = mesh->mNormals[j];
                v.normal = {n.x, n.y, n.z};
            } else {
                v.normal = {0.0f, 1.0f, 0.0f};  // 更合理的默认法线（朝上）
            }

            // Vertex Color
            if (mesh->HasVertexColors(0)) {
                const aiColor4D& c = mesh->mColors[0][j];
                v.color = {c.r, c.g, c.b};
            } else {
                v.color = {1.0f, 1.0f, 1.0f};
            }

            model.vertices_.push_back(v);
        }

        // === 2. 加载索引（关键：使用 vertexOffset）===
        for (unsigned int f = 0; f < mesh->mNumFaces; ++f) {
            const aiFace& face = mesh->mFaces[f];
            if (face.mNumIndices != 3) {
                continue;  // 跳过非三角面（或报错）
            }

            for (unsigned int idx = 0; idx < 3; ++idx) {
                uint32_t localVertexIndex = face.mIndices[idx];
                uint32_t globalVertexIndex = localVertexIndex + vertexOffset;  // ✅ 修复点！
                model.indices_.push_back(globalVertexIndex);
            }
        }

        // === 3. 创建 SubMesh ===
        graphics::SubMesh sub;
        sub.indexOffset = indexOffset;
        sub.indexCount = mesh->mNumFaces * 3;

        // === 4. 加载材质 ===
        graphics::MeshMaterial mat;
        if (scene->HasMaterials() && mesh->mMaterialIndex < scene->mNumMaterials) {
            aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

            // 名称
            aiString name;
            if (material->Get(AI_MATKEY_NAME, name) == AI_SUCCESS) {
                mat.name = name.C_Str();
            }

            // 颜色
            aiColor3D ka(1.f, 1.f, 1.f), kd(1.f, 1.f, 1.f), ks(1.f, 1.f, 1.f), ke(0.f, 0.f, 0.f);
            material->Get(AI_MATKEY_COLOR_AMBIENT, ka);
            mat.ambientColor = {ka.r, ka.g, ka.b};
            material->Get(AI_MATKEY_COLOR_DIFFUSE, kd);
            mat.diffuseColor = {kd.r, kd.g, kd.b};
            material->Get(AI_MATKEY_COLOR_SPECULAR, ks);
            mat.specularColor = {ks.r, ks.g, ks.b};
            material->Get(AI_MATKEY_COLOR_EMISSIVE, ke);
            mat.emissiveColor = {ke.r, ke.g, ke.b};

            // 标量
            material->Get(AI_MATKEY_SHININESS, mat.shininess);
            material->Get(AI_MATKEY_OPACITY, mat.opacity);
            material->Get(AI_MATKEY_REFRACTI, mat.ior);
            ai_real value = NAN;
            if (material->Get(AI_MATKEY_METALLIC_FACTOR, value) == AI_SUCCESS) {
                mat.metallic = static_cast<float>(value);
            }

            ai_real roughness_value = NAN;
            if (material->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness_value) == AI_SUCCESS) {
                mat.roughness = static_cast<float>(roughness_value);
            }

            // 纹理
            mat.ambientTextures = getTexturePath(material, aiTextureType_AMBIENT);
            mat.diffuseTextures = getTexturePath(material, aiTextureType_DIFFUSE);
            mat.specularTextures = getTexturePath(material, aiTextureType_SPECULAR);
            mat.heightTextures = getTexturePath(material, aiTextureType_HEIGHT);
            mat.normalTextures = getTexturePath(material, aiTextureType_NORMALS);
            if (mat.normalTextures.empty()) {
                mat.normalTextures = mat.heightTextures;
            }
            mat.emissiveTextures = getTexturePath(material, aiTextureType_EMISSIVE);

            // ========== PBR 贴图（Assimp 5.0+ 支持）==========

            // Albedo (Base Color) —— PBR 中的 diffuse 替代
            mat.albedoTextures = getTexturePath(material, aiTextureType_BASE_COLOR);
            if (mat.albedoTextures.empty()) {
                mat.albedoTextures = mat.diffuseTextures;
            }
            // Metallic
            mat.metallicTextures = getTexturePath(material, aiTextureType_METALNESS);
            // Roughness
            mat.metallicRoughnessTextures =
                getTexturePath(material, aiTextureType_DIFFUSE_ROUGHNESS);

            mat.aoTextures = getTexturePath(material, aiTextureType_AMBIENT_OCCLUSION);
            if (mat.aoTextures.empty()) {
                mat.aoTextures = mat.ambientTextures;
            }
        }

        sub.material = mat;
        model.subMeshes.push_back(sub);

        // === 5. 更新偏移 ===
        vertexOffset += mesh->mNumVertices;
        indexOffset += sub.indexCount;
    }

    return model;
}

}  // namespace

namespace graphics {

auto Model::createFromFile(const ::std::string& path, std::uint64_t obj_hash) -> Model {
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
    constexpr auto ASSIMP_LOAD_FLAGS = aiProcess_Triangulate | aiProcess_FlipUVs |
                                       aiProcess_JoinIdenticalVertices |
                                       aiProcess_GenNormals;
    // NOLINTNEXTLINE
    const aiScene* scene = importer.ReadFile(path, ASSIMP_LOAD_FLAGS);
    if (!scene || !scene->HasMeshes() || !scene->mRootNode ||
        scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) {
        throw std::runtime_error("load empty model");
    }
    Model model = loadModelFromAssimpScene(scene);
    std::string cache_path = model_cache_path + std::to_string(obj_hash) + model_cache_extend;
    saveModelToCache(obj_hash, model);

    return model;
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
