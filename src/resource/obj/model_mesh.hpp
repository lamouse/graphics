#pragma once

#include "resource/obj/mesh_material.hpp"

#include <vector>
#include <string>
#include <glm/gtx/hash.hpp>
#include "mesh.hpp"
#include "render_core/pipeline_state.h"
#include <assimp/scene.h>
namespace graphics {

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
