#pragma once

#include "resource/obj/mesh_material.hpp"
#include "resource/obj/mesh_vertex.hpp"
#include "render_core/pipeline_state.h"
#include "render_core/mesh.hpp"

#include <vector>
#include <string>
#include <iostream>

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

class Model : public render::IMeshData {
    public:
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
        std::vector<Vertex> vertices_;
        std::vector<SubMesh> subMeshes;
        Model() = default;
};

class MultiMeshModel {
    public:
        struct Mesh : public render::IMeshData {
                std::vector<Vertex> vertices_;
                std::vector<::glm::vec3> only_vertex;
                std::vector<uint32_t> indices_;
                MeshMaterial material;
                [[nodiscard]] auto getMesh() const -> std::span<const float> override {
                    return std::span<const float>(
                        reinterpret_cast<const float*>(vertices_.data()),
                        vertices_.size() * sizeof(Vertex) / sizeof(float));
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
                    return Vertex::getVertexAttribute();
                }
                [[nodiscard]] auto getVertexBinding() const
                    -> std::vector<render::VertexBinding> override {
                    return Vertex::getVertexBinding();
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
                                 sizeof(Vertex) * vcount);
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
                                               sizeof(Vertex) * vcount)) {
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
