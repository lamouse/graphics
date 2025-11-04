#include "model.hpp"

#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <assimp/Importer.hpp>
#include <cassert>
#include <format>
#include <unordered_map>

namespace {
template <typename T>
std::span<const std::byte> as_bytes(std::span<const T> s) {
    // 将 T* 指针 reinterpret_cast 为 const std::byte* 指针
    auto ptr = reinterpret_cast<const std::byte*>(s.data());
    // 第二个参数是 span 的“元素”数量，即字节数
    return std::span<const std::byte>(ptr, s.size() * sizeof(T));
}

}  // namespace

namespace graphics {

Model::Model(const ::std::vector<Vertex>& vertices, const ::std::vector<uint16_t>& indices)
    : vertexCount(static_cast<uint32_t>(vertices.size())),
      indicesSize(static_cast<uint32_t>(indices.size())),
      vertices_(vertices),
      u16_indices_(indices) {
    for (const auto& full_vertex : vertices) {
        only_vertex.push_back(full_vertex.position);
    }
    save32_indices.insert(save32_indices.begin(), indices.begin(), indices.end());

    assert(vertexCount >= 3 && "Vertex count must be at least 3");
}

Model::Model(const ::std::vector<Vertex>& vertices, const ::std::vector<uint32_t>& indices)
    : save32_indices(indices),
      vertexCount(static_cast<uint32_t>(vertices.size())),
      indicesSize(static_cast<uint32_t>(indices.size())),
      vertices_(vertices),
      u32_indices_(indices),
      use32BitIndices(true) {
    for (const auto& full_vertex : vertices) {
        only_vertex.push_back(full_vertex.position);
    }
    assert(vertexCount >= 3 && "Vertex count must be at least 3");
}

auto Model::createFromFile(const ::std::string& path) -> Model {
    std::vector<Model::Vertex> vertices;
    std::vector<uint32_t> u32_indices;

    Assimp::Importer importer;
    constexpr auto ASSIMP_LOAD_FLAGS =
        aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices;
    // NOLINTNEXTLINE
    const aiScene* scene = importer.ReadFile(path, ASSIMP_LOAD_FLAGS);
    if (!scene) {
        throw std::runtime_error(
            std::format("Failed to load model: {}", importer.GetErrorString()));
    }
    for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[i];
        for (unsigned int j = 0; j < mesh->mNumVertices; j++) {
            aiVector3D vertex = mesh->mVertices[j];
            Model::Vertex model_vertex{};
            model_vertex.position = {vertex.x, vertex.y, vertex.z};

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

        u32_indices.reserve(mesh->mNumFaces * 3);
        for (u32 f = 0; f < mesh->mNumFaces; f++) {
            const auto face = mesh->mFaces[f];
            ASSERT_MSG(face.mNumIndices == 3, "face indices not equal 3");
            for (unsigned int idx = 0; idx < 3; ++idx) {
                uint32_t vertexIndex = face.mIndices[idx];
                u32_indices.push_back(vertexIndex);
            }
        }
    }
    // 判断是否需要使用 32 位索引
    if (vertices.size() > std::numeric_limits<uint16_t>::max()) {
        // 使用 32 位索引
        return Model(vertices,
                     u32_indices);  // 你需要添加一个支持 uint32_t 的构造函数
    }
    // 转换为 16 位索引
    std::vector<uint16_t> u16_indices;
    u16_indices.resize(u32_indices.size());
    std::ranges::transform(u32_indices, u16_indices.begin(),
                           [](uint32_t idx) { return static_cast<uint16_t>(idx); });
    return Model(vertices, u16_indices);
}

// 返回顶点坐标（仅 position），展平为 float 数组
[[nodiscard]] auto Model::getMesh() const -> std::span<const float> {
    return std::span<const float>(reinterpret_cast<const float*>(vertices_.data()),
                                  vertices_.size() * sizeof(Model::Vertex) / sizeof(float));
}

auto Model::getIndices() const -> std::span<const std::byte> {
    if (use32BitIndices) {
        return as_bytes(std::span(u32_indices_));
    }

    return as_bytes(std::span(u16_indices_));
}

}  // namespace graphics
