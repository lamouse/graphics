#include "model_mesh.hpp"

#include "common/file.hpp"
#include <assimp/postprocess.h>

#include <assimp/Importer.hpp>
#include <cassert>
#include <fstream>
#include <stack>

namespace {
constexpr const char* model_cache_path = "data/cache/mesh/";
constexpr const char* model_cache_extend = ".mesh";
constexpr const char* model_multi_mesh_cache_extend = ".meshes";
constexpr auto DEFAULT_ULT_ASSIMP_LOAD_FLAGS = aiProcess_Triangulate |
                                               aiProcess_JoinIdenticalVertices |
                                               aiProcess_GenNormals | aiProcess_EmbedTextures;
constexpr auto FLIP_UV_ULT_ASSIMP_LOAD_FLAGS = aiProcess_Triangulate | aiProcess_FlipUVs |
                                               aiProcess_JoinIdenticalVertices |
                                               aiProcess_GenNormals | aiProcess_EmbedTextures;

constexpr uint32_t MULTIMESH_CACHE_MAGIC = 0x4D4D5348;  // 'MMSH'

struct MultiMeshCacheHeader {
        uint32_t magic = MULTIMESH_CACHE_MAGIC;
        uint32_t version = graphics::MESH_CACHE_VERSION;
        uint64_t fileHash = 0;
        uint32_t meshCount = 0;
        uint32_t padding = 0;
};
template <typename T>
auto as_bytes(std::span<const T> s) -> std::span<const std::byte> {
    // 将 T* 指针 reinterpret_cast 为 const std::byte* 指针
    const auto* ptr = reinterpret_cast<const std::byte*>(s.data());
    // 第二个参数是 span 的“元素”数量，即字节数
    return std::span<const std::byte>(ptr, s.size() * sizeof(T));
}

void saveModelToCache(std::uint64_t file_hash, const graphics::Model& model) {
    common::FS::create_dir(model_cache_path);
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
                   static_cast<long long>(sizeof(graphics::Vertex) * vcount));
    }
    if (icount > 0) {
        file.write(reinterpret_cast<const char*>(model.indices_.data()),
                   static_cast<long long>(sizeof(uint32_t) * icount));
    }
    if (ocount > 0) {
        file.write(reinterpret_cast<const char*>(model.only_vertex.data()),
                   static_cast<long long>(sizeof(glm::vec3) * ocount));
    }

    // 写入 sub meshes
    for (const auto& sub : model.subMeshes) {
        file.write(reinterpret_cast<const char*>(&sub.indexOffset), sizeof(sub.indexOffset));
        file.write(reinterpret_cast<const char*>(&sub.indexCount), sizeof(sub.indexCount));
        file.write(reinterpret_cast<const char*>(&sub.primitiveTopology),
                   sizeof(sub.primitiveTopology));
        sub.material.serialize(file);
    }
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
    std::vector<graphics::Vertex> vertices(vertexCount);
    std::vector<uint32_t> indices(indexCount);
    std::vector<glm::vec3> only_vertices(onlyVertexCount);

    file.read(reinterpret_cast<char*>(vertices.data()),
              static_cast<long long>(sizeof(graphics::Vertex) * vertexCount));
    file.read(reinterpret_cast<char*>(indices.data()),
              static_cast<long long>(sizeof(uint32_t) * indexCount));
    file.read(reinterpret_cast<char*>(only_vertices.data()),
              static_cast<long long>(sizeof(glm::vec3) * onlyVertexCount));

    // 读取所有 SubMesh
    std::vector<graphics::SubMesh> subMeshes;
    subMeshes.reserve(header.subMeshCount);
    for (uint32_t i = 0; i < header.subMeshCount; ++i) {
        graphics::SubMesh sub;
        file.read(reinterpret_cast<char*>(&sub.indexOffset), sizeof(sub.indexOffset));
        file.read(reinterpret_cast<char*>(&sub.indexCount), sizeof(sub.indexCount));
        file.read(reinterpret_cast<char*>(&sub.primitiveTopology), sizeof(sub.primitiveTopology));
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
            graphics::Vertex v{};

            // Position
            const aiVector3D& pos = mesh->mVertices[j];
            v.position = {pos.x, pos.y, pos.z};
            model.only_vertex.push_back(v.position);

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
                v.normal = {0.0f, 1.0f, 0.0f};
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
        render::PrimitiveTopology topology = render::PrimitiveTopology::Triangles;
        uint32_t meshIndexCount = 0;
        // === 2. 加载索引（关键：使用 vertexOffset）===
        for (unsigned int f = 0; f < mesh->mNumFaces; ++f) {
            const aiFace& face = mesh->mFaces[f];
            if (face.mNumIndices == 3) {
                topology = render::PrimitiveTopology::Triangles;  // 跳过非三角面（或报错）
            } else if (face.mNumIndices == 2) {
                topology = render::PrimitiveTopology::Lines;
            }

            for (unsigned int idx = 0; idx < face.mNumIndices; ++idx) {
                uint32_t localVertexIndex = face.mIndices[idx];
                uint32_t globalVertexIndex = localVertexIndex + vertexOffset;
                model.indices_.push_back(globalVertexIndex);
            }
            meshIndexCount += face.mNumIndices;
        }

        // === 3. 创建 SubMesh ===
        graphics::SubMesh sub{.indexOffset = indexOffset,
                              .indexCount = meshIndexCount,
                              .primitiveTopology = topology,
                              .material = graphics::loadMaterial(scene, mesh)};
        model.subMeshes.push_back(sub);

        // === 5. 更新偏移 ===
        vertexOffset += mesh->mNumVertices;
        indexOffset += sub.indexCount;
    }

    return model;
}

void saveMultiMeshToCache(uint64_t file_hash, const graphics::MultiMeshModel& model) {
    common::FS::create_dir(model_cache_path);
    auto cachePath =
        std::string(model_cache_path) + std::to_string(file_hash) + model_multi_mesh_cache_extend;
    std::ofstream file(cachePath, std::ios::binary);
    if (!file) {
        return;
    }

    MultiMeshCacheHeader header{};
    header.magic = MULTIMESH_CACHE_MAGIC;
    header.version = graphics::MESH_CACHE_VERSION;
    header.fileHash = file_hash;
    auto meshes = model.getMeshes();
    header.meshCount = static_cast<uint32_t>(meshes.size());

    file.write(reinterpret_cast<const char*>(&header), sizeof(header));

    for (const auto& mesh : meshes) {
        mesh.serialize(file);
    }
}

auto loadMultiMeshFromCache(uint64_t file_hash) -> std::vector<graphics::MultiMeshModel::Mesh> {
    auto cachePath =
        std::string(model_cache_path) + std::to_string(file_hash) + model_multi_mesh_cache_extend;
    std::ifstream file(cachePath, std::ios::binary);
    if (!file) {
        return {};
    }

    MultiMeshCacheHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(header));
    if (header.magic != MULTIMESH_CACHE_MAGIC || header.version != graphics::MESH_CACHE_VERSION ||
        header.fileHash != file_hash) {
        return {};
    }

    std::vector<graphics::MultiMeshModel::Mesh> meshes;
    meshes.reserve(header.meshCount);

    for (uint32_t i = 0; i < header.meshCount; ++i) {
        graphics::MultiMeshModel::Mesh mesh;
        if (!mesh.deserialize(file)) {
            return {};
        }
        meshes.push_back(std::move(mesh));
    }

    return meshes;
}

}  // namespace

namespace graphics {

auto Model::createFromFile(const ::std::string& model_path, std::uint64_t obj_hash, bool flip_uv)
    -> Model {
    auto importer_flag = DEFAULT_ULT_ASSIMP_LOAD_FLAGS;
    if (flip_uv) {
        importer_flag = FLIP_UV_ULT_ASSIMP_LOAD_FLAGS;
    }

    if (obj_hash == 0) {
        auto file_hash = common::FS::file_hash(model_path);
        obj_hash = file_hash ? file_hash.value() : 0;
    }
    if (!obj_hash) {
        return {};
    }
    if (auto meshes = loadModelWithCache(obj_hash)) {
        return std::move(meshes.value());
    }

    Assimp::Importer importer;

    // NOLINTNEXTLINE
    const aiScene* scene = importer.ReadFile(model_path, importer_flag);
    if (!scene || !scene->HasMeshes() || !scene->mRootNode ||
        scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) {
        throw std::runtime_error("load model fail: " + std::string(importer.GetErrorString()));
    }
    Model model = loadModelFromAssimpScene(scene);
    std::string cache_path = model_cache_path + std::to_string(obj_hash) + model_cache_extend;
    saveModelToCache(obj_hash, model);

    return model;
}

// 返回顶点坐标（仅 position），展平为 float 数组
[[nodiscard]] auto Model::getMesh() const -> std::span<const float> {
    return std::span<const float>(reinterpret_cast<const float*>(vertices_.data()),
                                  vertices_.size() * sizeof(Vertex) / sizeof(float));
}

auto Model::getIndices() const -> std::span<const std::byte> {
    return as_bytes(std::span(indices_));
}

MultiMeshModel::MultiMeshModel(std::string_view path, uint64_t file_hash_, bool flip_uv)
    : file_hash(file_hash_) {
    if (file_hash_ == 0) {
        auto model_file_hash = common::FS::file_hash(std::string(path));
        file_hash = model_file_hash ? model_file_hash.value() : 0;
    }
    auto meshes = loadMultiMeshFromCache(file_hash);
    if (!meshes.empty()) {
        meshes_ = std::move(meshes);
        return;
    }
    Assimp::Importer importer;
    // NOLINTNEXTLINE
    const aiScene* scene = importer.ReadFile(
        std::string(path), flip_uv ? FLIP_UV_ULT_ASSIMP_LOAD_FLAGS : DEFAULT_ULT_ASSIMP_LOAD_FLAGS);
    if (!scene || !scene->HasMeshes() || !scene->mRootNode ||
        scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) {
        throw std::runtime_error("load model fail: " + std::string(importer.GetErrorString()));
    }
    processNode(scene->mRootNode, scene);
}

void MultiMeshModel::processNode(aiNode* root, const aiScene* scene) {
    if (!root) {
        return;
    }

    // 显式栈
    std::stack<aiNode*> stack;
    stack.push(root);

    while (!stack.empty()) {
        auto* node = stack.top();
        stack.pop();

        // 处理该节点的所有网格
        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            processMesh(mesh, scene);
        }

        // 将子节点压栈
        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            stack.push(node->mChildren[i]);
        }
    }
}
void MultiMeshModel::processMesh(aiMesh* mesh, const aiScene* scene) {
    Mesh m;

    // 处理顶点
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex{};

        // 位置
        const aiVector3D& pos = mesh->mVertices[i];
        vertex.position = {pos.x, pos.y, pos.z};
        m.only_vertex.push_back(vertex.position);

        // Vertex Color
        if (mesh->HasVertexColors(0)) {
            const aiColor4D& c = mesh->mColors[0][i];
            vertex.color = {c.r, c.g, c.b};
        } else {
            vertex.color = {1.0f, 1.0f, 1.0f};
        }

        // 纹理坐标
        if (mesh->HasTextureCoords(0)) {
            const aiVector3D& tex = mesh->mTextureCoords[0][i];
            vertex.texCoord = {tex.x, tex.y};
        } else {
            vertex.texCoord = {0.0f, 0.0f};
        }

        // 法线
        if (mesh->HasNormals()) {
            const aiVector3D& n = mesh->mNormals[i];
            vertex.normal = {n.x, n.y, n.z};
        } else {
            vertex.normal = {0.0f, 1.0f, 0.0f};
        }

        m.vertices_.push_back(vertex);
    }

    // 处理索引
    for (unsigned int f = 0; f < mesh->mNumFaces; f++) {
        const aiFace& face = mesh->mFaces[f];
        for (unsigned int idx = 0; idx < face.mNumIndices; idx++) {
            m.indices_.push_back(face.mIndices[idx]);
        }
    }

    // 处理材质
    m.material = loadMaterial(scene, mesh);

    meshes_.push_back(std::move(m));
}

MultiMeshModel::~MultiMeshModel() { saveMultiMeshToCache(file_hash, *this); }

}  // namespace graphics
