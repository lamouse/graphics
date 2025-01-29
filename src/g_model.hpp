#pragma once

#include <glm/gtx/hash.hpp>
#include <memory>
#include <vector>

#include "core/buffer.hpp"
namespace g {
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
        void bind(const ::vk::CommandBuffer& commandBuffer) const;
        void draw(const ::vk::CommandBuffer& commandBuffer) const;
        Model(const ::std::vector<Vertex>& vertices, const ::std::vector<uint16_t>& indices);
        ~Model() = default;
        std::vector<Model::Vertex> vertices_;
        std::vector<uint16_t> indices_;

    private:
        // core::Buffer vertexBuffer_;
        // core::Buffer indexBuffer_;
        uint32_t vertexCount;
        uint32_t indicesSize;
};

}  // namespace g

namespace std {
template <>
struct hash<g::Model::Vertex> {
        auto operator()(g::Model::Vertex const& vertex) const -> size_t {
            return ((hash<glm::vec3>()(vertex.position) ^ (hash<glm::vec3>()(vertex.color) << 1)) >>
                    1) ^
                   (hash<glm::vec2>()(vertex.texCoord) << 1);
        }
};
}  // namespace std
