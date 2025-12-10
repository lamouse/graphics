#pragma once

#include <vector>
#include <string>
#include <glm/gtx/hash.hpp>
#include "mesh.hpp"
#include "render_core/pipeline_state.h"
#include <assimp/scene.h>
namespace graphics {

// 辅助 lambda：写入 vector<string>
const auto write_string_vector = [](std::ostream& os, const std::vector<std::string>& vec) -> void {
    auto size = static_cast<uint32_t>(vec.size());
    os.write(reinterpret_cast<const char*>(&size), sizeof(size));
    for (const auto& str : vec) {
        auto len = static_cast<uint32_t>(str.size());
        os.write(reinterpret_cast<const char*>(&len), sizeof(len));
        if (len > 0) {
            os.write(str.data(), len);
        }
    }
};

// 辅助 lambda：读取 vector<string>
const auto read_string_vector = [](std::istream& is, std::vector<std::string>& vec) -> bool {
    uint32_t size = 0;
    if (!is.read(reinterpret_cast<char*>(&size), sizeof(size))) {
        return false;
    }
    vec.resize(size);
    for (auto& str : vec) {
        uint32_t len = 0;
        if (!is.read(reinterpret_cast<char*>(&len), sizeof(len))) {
            return false;
        }
        if (len > 0) {
            str.resize(len);
            if (!is.read(str.data(), len)) {
                return false;
            }
        } else {
            str.clear();
        }
    }
    return true;
};
constexpr const uint32_t MESH_CACHE_VERSION = 1;
constexpr uint32_t MODEL_CACHE_MAGIC = 0x4D4F444C;  // 'MODL'
struct ModelCacheHeader {
        static constexpr uint32_t MAGIC = MODEL_CACHE_MAGIC;

        uint32_t magic = MAGIC;
        uint32_t version = MESH_CACHE_VERSION;
        uint64_t objFileHash = 0;
        uint32_t subMeshCount = 0;  // 改为 subMeshCount
        uint32_t padding = 0;       // 对齐
};

struct MeshMaterial {
        std::string name = "default";

        // 颜色属性（来自 .mtl）
        glm::vec3 ambientColor = {1.0f, 1.0f, 1.0f};   // Ka
        glm::vec3 diffuseColor = {1.0f, 1.0f, 1.0f};   // Kd
        glm::vec3 specularColor = {.04f, .04f, .04f};  // Ks
        glm::vec3 emissiveColor = {0.0f, 0.0f, 0.0f};  // Ke

        // 标量属性
        float shininess = 32.0f;  // Ns
        float opacity = 1.0f;     // d
        float ior = 1.0f;         // Ni

        // PBR
        float metallic = 0.0f;
        float roughness = .5f;
        float ao = 1.0f;

        // 纹理路径 —— 改为 vector 支持多贴图
        std::vector<std::string> ambientTextures;   // map_Ka
        std::vector<std::string> diffuseTextures;   // map_Kd
        std::vector<std::string> specularTextures;  // map_Ks
        std::vector<std::string> normalTextures;    // map_Bump / map_Ns
        std::vector<std::string> emissiveTextures;  // map_Ke
        std::vector<std::string> aoTextures;
        std::vector<std::string> metallicTextures;
        std::vector<std::string> albedoTextures;
        std::vector<std::string> metallicRoughnessTextures;
        std::vector<std::string> heightTextures;

        // ----------------------------
        // 序列化：写入二进制流
        // ----------------------------
        void serialize(std::ostream& os) const {
            // 写入颜色
            os.write(reinterpret_cast<const char*>(&ambientColor), sizeof(ambientColor));
            os.write(reinterpret_cast<const char*>(&diffuseColor), sizeof(diffuseColor));
            os.write(reinterpret_cast<const char*>(&specularColor), sizeof(specularColor));
            os.write(reinterpret_cast<const char*>(&emissiveColor), sizeof(emissiveColor));

            // 写入标量
            os.write(reinterpret_cast<const char*>(&shininess), sizeof(shininess));
            os.write(reinterpret_cast<const char*>(&opacity), sizeof(opacity));
            os.write(reinterpret_cast<const char*>(&ior), sizeof(ior));

            // 写入PBR
            os.write(reinterpret_cast<const char*>(&metallic), sizeof(metallic));
            os.write(reinterpret_cast<const char*>(&roughness), sizeof(roughness));
            os.write(reinterpret_cast<const char*>(&ao), sizeof(ao));

            // 写入 name（保持为单 string）
            {
                auto len = static_cast<uint32_t>(name.size());
                os.write(reinterpret_cast<const char*>(&len), sizeof(len));
                if (len > 0) os.write(name.data(), len);
            }

            // 写入所有纹理 vector
            write_string_vector(os, ambientTextures);
            write_string_vector(os, diffuseTextures);
            write_string_vector(os, specularTextures);
            write_string_vector(os, normalTextures);
            write_string_vector(os, emissiveTextures);
            write_string_vector(os, aoTextures);
            write_string_vector(os, metallicTextures);
            write_string_vector(os, albedoTextures);
            write_string_vector(os, metallicRoughnessTextures);
            write_string_vector(os, heightTextures);
        }

        // ----------------------------
        // 反序列化：从二进制流读取
        // ----------------------------
        [[nodiscard]] auto deserialize(std::istream& is) -> bool {
            // 读取颜色
            if (!is.read(reinterpret_cast<char*>(&ambientColor), sizeof(ambientColor)))
                return false;
            if (!is.read(reinterpret_cast<char*>(&diffuseColor), sizeof(diffuseColor)))
                return false;
            if (!is.read(reinterpret_cast<char*>(&specularColor), sizeof(specularColor)))
                return false;
            if (!is.read(reinterpret_cast<char*>(&emissiveColor), sizeof(emissiveColor)))
                return false;

            // 读取标量
            if (!is.read(reinterpret_cast<char*>(&shininess), sizeof(shininess))) return false;
            if (!is.read(reinterpret_cast<char*>(&opacity), sizeof(opacity))) return false;
            if (!is.read(reinterpret_cast<char*>(&ior), sizeof(ior))) return false;
            if (!is.read(reinterpret_cast<char*>(&metallic), sizeof(metallic))) return false;
            if (!is.read(reinterpret_cast<char*>(&roughness), sizeof(roughness))) return false;
            if (!is.read(reinterpret_cast<char*>(&ao), sizeof(ao))) return false;

            // 读取 name
            {
                uint32_t len = 0;
                if (!is.read(reinterpret_cast<char*>(&len), sizeof(len))) return false;
                if (len > 0) {
                    name.resize(len);
                    if (!is.read(name.data(), len)) return false;
                } else {
                    name.clear();
                }
            }

            // 读取所有纹理 vector
            if (!read_string_vector(is, ambientTextures)) return false;
            if (!read_string_vector(is, diffuseTextures)) return false;
            if (!read_string_vector(is, specularTextures)) return false;
            if (!read_string_vector(is, normalTextures)) return false;
            if (!read_string_vector(is, emissiveTextures)) return false;
            if (!read_string_vector(is, aoTextures)) return false;
            if (!read_string_vector(is, metallicTextures)) return false;
            if (!read_string_vector(is, albedoTextures)) return false;
            if (!read_string_vector(is, metallicRoughnessTextures)) return false;
            if (!read_string_vector(is, heightTextures)) return false;

            return true;
        }
};

struct SubMesh {
        uint32_t indexOffset = 0;
        uint32_t indexCount = 0;
        render::PrimitiveTopology primitiveTopology{render::PrimitiveTopology::Triangles};
        MeshMaterial material;  // 每个子网格有自己的材质
};

class Model : public IMeshData {
    public:
        struct Vertex {
                ::glm::vec3 position;
                ::glm::vec3 color;
                ::glm::vec3 normal;
                ::glm::vec2 texCoord;
                auto operator==(const Vertex& other) const -> bool {
                    return position == other.position && color == other.color &&
                           texCoord == other.texCoord;
                }

                static auto getVertexBinding() -> std::vector<render::VertexBinding> {
                    std::vector<render::VertexBinding> bindings;
                    bindings.push_back(render::VertexBinding{.binding = 0,
                                                             .stride = sizeof(Vertex),
                                                             .is_instance = false,
                                                             .divisor = 1});
                    return bindings;
                }
                static auto getVertexAttribute() -> std::vector<render::VertexAttribute> {
                    std::vector<render::VertexAttribute> vertex_attributes;

                    render::VertexAttribute position;
                    position.hex = 0;
                    position.location.Assign(0);
                    position.type.Assign(render::VertexAttribute::Type::Float);
                    position.offset.Assign(offsetof(Vertex, position));
                    position.size.Assign(render::VertexAttribute::Size::R32_G32_B32);
                    vertex_attributes.push_back(position);

                    render::VertexAttribute color;
                    color.hex = 0;
                    color.location.Assign(1);
                    color.type.Assign(render::VertexAttribute::Type::Float);
                    color.offset.Assign(offsetof(Vertex, color));
                    color.size.Assign(render::VertexAttribute::Size::R32_G32_B32);
                    vertex_attributes.push_back(color);

                    render::VertexAttribute normal;
                    normal.hex = 0;
                    normal.location.Assign(2);
                    normal.type.Assign(render::VertexAttribute::Type::Float);
                    normal.offset.Assign(offsetof(Vertex, normal));
                    normal.size.Assign(render::VertexAttribute::Size::R32_G32_B32);
                    vertex_attributes.push_back(normal);

                    render::VertexAttribute texCoord;
                    texCoord.hex = 0;
                    texCoord.location.Assign(3);
                    texCoord.type.Assign(render::VertexAttribute::Type::Float);
                    texCoord.offset.Assign(offsetof(Vertex, texCoord));
                    texCoord.size.Assign(render::VertexAttribute::Size::R32_G32);
                    vertex_attributes.push_back(texCoord);
                    return vertex_attributes;
                }
        };

        // 返回顶点坐标（仅 position），展平为 float 数组
        [[nodiscard]] auto getMesh() const -> std::span<const float> override;

        [[nodiscard]] auto getVertexCount() const -> std::size_t override {
            return vertices_.size();
        };

        [[nodiscard]] auto getIndices() const -> std::span<const std::byte> override;

        [[nodiscard]] auto getVertexAttribute() const
            -> std::vector<render::VertexAttribute> override {
            return Vertex::getVertexAttribute();
        }
        [[nodiscard]] auto getVertexBinding() const -> std::vector<render::VertexBinding> override {
            return Vertex::getVertexBinding();
        }
        static auto createFromFile(const ::std::string& path, std::uint64_t obj_hash,
                                   bool flip_uv = false) -> Model;
        CLASS_NON_COPYABLE(Model);
        CLASS_DEFAULT_MOVEABLE(Model);
        Model(const ::std::vector<Vertex>& vertices, const ::std::vector<uint32_t>& indices,
              const std::vector<::glm::vec3>& only_vertex, MeshMaterial material);
        [[nodiscard]] auto getIndicesSize() const -> std::uint64_t override {
            return indices_.size();
        }
        ~Model() override = default;
        std::vector<::glm::vec3> only_vertex;
        std::vector<uint32_t> indices_;
        std::vector<Model::Vertex> vertices_;
        std::vector<SubMesh> subMeshes;
        Model() = default;
};

class MultiMeshModel {
    public:
        struct Mesh : public IMeshData {
                std::vector<Model::Vertex> vertices_;
                std::vector<::glm::vec3> only_vertex;
                std::vector<uint32_t> indices_;
                MeshMaterial material;
                [[nodiscard]] auto getMesh() const -> std::span<const float> override {
                    return std::span<const float>(
                        reinterpret_cast<const float*>(vertices_.data()),
                        vertices_.size() * sizeof(Model::Vertex) / sizeof(float));
                }
                [[nodiscard]] auto getVertexCount() const -> std::size_t override {
                    return vertices_.size();
                };
                [[nodiscard]] auto getIndices() const -> std::span<const std::byte> override {
                    return as_bytes(std::span(indices_));
                }
                [[nodiscard]] auto getIndicesSize() const -> std::uint64_t override {
                    return indices_.size();
                }
                [[nodiscard]] auto getVertexAttribute() const
                    -> std::vector<render::VertexAttribute> override {
                    return Model::Vertex::getVertexAttribute();
                }
                [[nodiscard]] auto getVertexBinding() const
                    -> std::vector<render::VertexBinding> override {
                    return Model::Vertex::getVertexBinding();
                }

                void serialize(std::ostream& os) const {
                    // 写入顶点数量和数据
                    uint64_t vcount = vertices_.size();
                    uint64_t icount = indices_.size();
                    uint64_t ocount = only_vertex.size();

                    os.write(reinterpret_cast<const char*>(&vcount), sizeof(vcount));
                    os.write(reinterpret_cast<const char*>(&icount), sizeof(icount));
                    os.write(reinterpret_cast<const char*>(&ocount), sizeof(ocount));

                    if (vcount > 0) {
                        os.write(reinterpret_cast<const char*>(vertices_.data()),
                                 sizeof(Model::Vertex) * vcount);
                    }
                    if (icount > 0) {
                        os.write(reinterpret_cast<const char*>(indices_.data()),
                                 sizeof(uint32_t) * icount);
                    }
                    if (ocount > 0) {
                        os.write(reinterpret_cast<const char*>(only_vertex.data()),
                                 sizeof(glm::vec3) * ocount);
                    }

                    // 序列化材质
                    material.serialize(os);
                }

                [[nodiscard]] auto deserialize(std::istream& is) -> bool {
                    uint64_t vcount = 0, icount = 0, ocount = 0;
                    if (!is.read(reinterpret_cast<char*>(&vcount), sizeof(vcount))) return false;
                    if (!is.read(reinterpret_cast<char*>(&icount), sizeof(icount))) return false;
                    if (!is.read(reinterpret_cast<char*>(&ocount), sizeof(ocount))) return false;

                    vertices_.resize(vcount);
                    indices_.resize(icount);
                    only_vertex.resize(ocount);

                    if (vcount > 0 && !is.read(reinterpret_cast<char*>(vertices_.data()),
                                               sizeof(Model::Vertex) * vcount)) {
                        return false;
                    }
                    if (icount > 0 && !is.read(reinterpret_cast<char*>(indices_.data()),
                                               sizeof(uint32_t) * icount)) {
                        return false;
                    }
                    if (ocount > 0 && !is.read(reinterpret_cast<char*>(only_vertex.data()),
                                               sizeof(glm::vec3) * ocount)) {
                        return false;
                    }

                    return material.deserialize(is);
                }
        };

        [[nodiscard]] auto getMeshes() const -> std::span<const Mesh> { return meshes_; }
        MultiMeshModel(std::string_view path, uint64_t file_hash = 0, bool flip_uv = false);
        CLASS_NON_COPYABLE(MultiMeshModel);
        CLASS_DEFAULT_MOVEABLE(MultiMeshModel);
        ~MultiMeshModel();

    private:
        void processNode(aiNode* node, const aiScene* scene);
        void processMesh(aiMesh* mesh, const aiScene* scene);
        std::vector<Mesh> meshes_;
        uint64_t file_hash;
};

}  // namespace graphics

namespace std {
template <>
struct hash<graphics::Model::Vertex> {
        auto operator()(graphics::Model::Vertex const& vertex) const -> size_t {
            return ((hash<glm::vec3>()(vertex.position) ^ (hash<glm::vec3>()(vertex.color) << 1)) >>
                    1) ^
                   (hash<glm::vec2>()(vertex.texCoord) << 1);
        }
};
}  // namespace std
