#pragma once

#include <memory>
#include <vector>
#include <string>
#include <glm/gtx/hash.hpp>
#include "mesh.hpp"
namespace graphics {
class Model : public IMeshData {
    public:
        struct Vertex {
                ::glm::vec3 position;
                ::glm::vec3 color;
                ::glm::vec2 texCoord;
                auto operator==(const Vertex& other) const -> bool {
                    return position == other.position && color == other.color &&
                           texCoord == other.texCoord;
                }
        };

        // 返回顶点坐标（仅 position），展平为 float 数组
        [[nodiscard]] auto getMesh() const -> std::span<const float> override;

        // 返回索引数据（16位或32位）
        [[nodiscard]] auto getIndices16() const -> std::span<const uint16_t> override;
        [[nodiscard]] auto getIndices32() const -> std::span<const uint32_t> override;

        // 判断是否使用 32 位索引
        [[nodiscard]] auto uses32BitIndices() const -> bool override { return use32BitIndices; };
        static auto createFromFile(const ::std::string& path) -> ::std::unique_ptr<Model>;
        CLASS_NON_COPYABLE(Model);
        CLASS_NON_MOVEABLE(Model);
        Model(const ::std::vector<Vertex>& vertices, const ::std::vector<uint16_t>& indices);
        Model(const ::std::vector<Vertex>& vertices, const ::std::vector<uint32_t>& indices);
        [[nodiscard]] auto getIndicesSize() const -> std::uint64_t override {
            if (use32BitIndices) {
                return u32_indices_.size();
            }
            return u16_indices_.size();
        }
        ~Model() override = default;

    private:
        uint32_t vertexCount;
        uint32_t indicesSize;
        std::vector<Model::Vertex> vertices_;
        std::vector<uint16_t> u16_indices_;
        std::vector<uint32_t> u32_indices_;
        bool use32BitIndices{false};
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
