#pragma once
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
namespace render {
struct Vertex {
        ::glm::vec3 position;
        ::glm::vec3 color;
        ::glm::vec2 texCoord;
        auto operator==(const Vertex& other) const -> bool {
            return position == other.position && color == other.color && texCoord == other.texCoord;
        }
};

}  // namespace render
namespace std {
template <>
struct hash<render::Vertex> {
        auto operator()(render::Vertex const& vertex) const -> size_t {
            return ((hash<glm::vec3>()(vertex.position) ^ (hash<glm::vec3>()(vertex.color) << 1)) >>
                    1) ^
                   (hash<glm::vec2>()(vertex.texCoord) << 1);
        }
};
}  // namespace std

namespace render {
struct UniformBufferObject {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
};

}  // namespace render