#pragma once

#include <memory>
#include <vector>
#include <string>
#include <glm/gtx/hash.hpp>
#include "mesh.hpp"
namespace graphics {

struct ModelCacheHeader {
        static constexpr uint32_t MAGIC = 0x4D4F4443;  // 'MODC' (Model Cache)
        uint32_t magic = MAGIC;
        uint32_t version = 1;
        uint64_t objFileHash = 0;  // xxHash64 of .obj content
        uint32_t modelCount = 0;
};

// Per-model metadata
struct ModelDesc {
        uint64_t vertexCount = 0;
        uint64_t indexCount = 0;
        uint64_t onlyVertexCount = 0;

        // Offsets are in ELEMENTS (not bytes) for simplicity
        uint64_t vertexOffset = 0;      // into global vertices array
        uint64_t indexOffset = 0;       // into global indices array
        uint64_t onlyVertexOffset = 0;  // into global only_vertex array
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

        [[nodiscard]] auto getVertexCount() const -> std::size_t override { return vertexCount; };

        // 返回索引数据（16位或32位）
        [[nodiscard]] auto getIndices() const -> std::span<const std::byte> override;

        // 判断是否使用 32 位索引
        [[nodiscard]] auto getVertexAttribute() const
            -> std::vector<render::VertexAttribute> override {
            return Vertex::getVertexAttribute();
        }
        [[nodiscard]] auto getVertexBinding() const -> std::vector<render::VertexBinding> override {
            return Vertex::getVertexBinding();
        }
        static auto createFromFile(const ::std::string& path, std::uint64_t obj_hash) -> std::vector<Model>;
        CLASS_NON_COPYABLE(Model);
        CLASS_DEFAULT_MOVEABLE(Model);
        Model(const ::std::vector<Vertex>& vertices, const ::std::vector<uint32_t>& indices,
              const std::vector<::glm::vec3>& only_vertex);
        [[nodiscard]] auto getIndicesSize() const -> std::uint64_t override { return indicesSize; }
        ~Model() override = default;
        std::vector<::glm::vec3> only_vertex;
        std::vector<uint32_t> indices_;
        std::vector<Model::Vertex> vertices_;

    private:
        uint32_t vertexCount;
        uint32_t indicesSize;
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
