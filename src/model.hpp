#pragma once

#include <glm/gtx/hash.hpp>
#include <memory>
#include <vector>

#include "core/buffer.hpp"
namespace graphics {
class Model {
    public:
        struct Vertex {
                ::glm::vec3 position;
                ::glm::vec3 color;
                ::glm::vec2 texCoord;
                static auto getBindingDescription()
                    -> ::std::vector<::vk::VertexInputBindingDescription>;
                static auto getAttributeDescription()
                    -> ::std::vector<::vk::VertexInputAttributeDescription>;
                auto operator==(const Vertex& other) const -> bool {
                    return position == other.position && color == other.color &&
                           texCoord == other.texCoord;
                }
        };

        static auto createFromFile(const ::std::string& path) -> ::std::unique_ptr<Model>;
        Model(const Model&) = delete;
        auto operator=(const Model&) -> Model& = delete;
        Model(const ::std::vector<Vertex>& vertices, const ::std::vector<uint16_t>& indices);
        ~Model() = default;
        std::vector<Model::Vertex> vertices_;
        std::vector<uint16_t> indices_;

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
