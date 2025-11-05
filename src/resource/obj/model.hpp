#pragma once

#include <vector>
#include <string>
#include <glm/gtx/hash.hpp>
#include "mesh.hpp"
namespace graphics {
constexpr const uint32_t MESH_CACHE_VERSION = 3;
constexpr uint32_t MODEL_CACHE_MAGIC = 0x4D4F444C; // 'MODL'
struct ModelCacheHeader {
 static constexpr uint32_t MAGIC = MODEL_CACHE_MAGIC;

    uint32_t magic = MAGIC;
    uint32_t version = MESH_CACHE_VERSION;
    uint64_t objFileHash = 0;
    uint32_t subMeshCount = 0; // 改为 subMeshCount
    uint32_t padding = 0;      // 对齐
};
struct MeshMaterial {
        std::string name = "default";
        // 颜色属性（来自 .mtl）
        glm::vec3 ambientColor = {1.0f, 1.0f, 1.0f};   // Ka
        glm::vec3 diffuseColor = {1.0f, 1.0f, 1.0f};   // Kd
        glm::vec3 specularColor = {1.0f, 1.0f, 1.0f};  // Ks
        glm::vec3 emissiveColor = {0.0f, 0.0f, 0.0f};  // Ke

        // 标量属性
        float shininess = 64.0f;  // Ns (specular exponent)
        float opacity = 1.0f;     // d (1.0 = opaque, 0.0 = fully transparent)
        float ior = 1.0f;         // Ni (index of refraction)

        // 纹理路径（相对路径，来自 .mtl 的 map_*）
        std::string ambientTexture;   // map_Ka
        std::string diffuseTexture;   // map_Kd
        std::string specularTexture;  // map_Ks
        std::string normalTexture;    // map_Bump or map_Ns
        std::string emissiveTexture;  // map_Ke (rare, but supported by some tools)

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

            // 写入字符串（长度 + 内容）
            auto write_string = [&](const std::string& str) {
                auto len = static_cast<uint32_t>(str.size());
                os.write(reinterpret_cast<const char*>(&len), sizeof(len));
                if (len > 0) {
                    os.write(str.data(), len);
                }
            };

            write_string(name);
            write_string(ambientTexture);
            write_string(diffuseTexture);
            write_string(specularTexture);
            write_string(normalTexture);
            write_string(emissiveTexture);
        }

        // ----------------------------
        // 反序列化：从二进制流读取
        // ----------------------------
        [[nodiscard]] auto deserialize(std::istream& is) -> bool {
            // 读取颜色
            if (!is.read(reinterpret_cast<char*>(&ambientColor), sizeof(ambientColor))) {
                return false;
            }
            if (!is.read(reinterpret_cast<char*>(&diffuseColor), sizeof(diffuseColor))) {
                return false;
            }
            if (!is.read(reinterpret_cast<char*>(&specularColor), sizeof(specularColor))) {
                return false;
            }
            if (!is.read(reinterpret_cast<char*>(&emissiveColor), sizeof(emissiveColor))) {
                return false;
            }

            // 读取标量
            if (!is.read(reinterpret_cast<char*>(&shininess), sizeof(shininess))) {
                return false;
            }
            if (!is.read(reinterpret_cast<char*>(&opacity), sizeof(opacity))) {
                return false;
            }
            if (!is.read(reinterpret_cast<char*>(&ior), sizeof(ior))) {
                return false;
            }

            // 读取字符串
            auto read_string = [&](std::string& str) -> bool {
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
                return true;
            };

            if (!read_string(name)) {
                return false;
            }
            if (!read_string(ambientTexture)) {
                return false;
            }
            if (!read_string(diffuseTexture)) {
                return false;
            }
            if (!read_string(specularTexture)) {
                return false;
            }
            if (!read_string(normalTexture)) {
                return false;
            }
            if (!read_string(emissiveTexture)) {
                return false;
            }

            return true;
        }
};

struct SubMesh {
        uint32_t indexOffset = 0;
        uint32_t indexCount = 0;
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

        [[nodiscard]] auto getVertexCount() const -> std::size_t override { return vertices_.size(); };

        [[nodiscard]] auto getIndices() const -> std::span<const std::byte> override;

        [[nodiscard]] auto getVertexAttribute() const
            -> std::vector<render::VertexAttribute> override {
            return Vertex::getVertexAttribute();
        }
        [[nodiscard]] auto getVertexBinding() const -> std::vector<render::VertexBinding> override {
            return Vertex::getVertexBinding();
        }
        static auto createFromFile(const ::std::string& path, std::uint64_t obj_hash)
            -> Model;
        CLASS_NON_COPYABLE(Model);
        CLASS_DEFAULT_MOVEABLE(Model);
        Model(const ::std::vector<Vertex>& vertices, const ::std::vector<uint32_t>& indices,
              const std::vector<::glm::vec3>& only_vertex, MeshMaterial material);
        [[nodiscard]] auto getIndicesSize() const -> std::uint64_t override { return indices_.size(); }
        ~Model() override = default;
        std::vector<::glm::vec3> only_vertex;
        std::vector<uint32_t> indices_;
        std::vector<Model::Vertex> vertices_;
        std::vector<SubMesh> subMeshes;
        Model() = default;
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
