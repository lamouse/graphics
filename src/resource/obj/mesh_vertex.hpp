#pragma once
#include "render_core/vertex.hpp"
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

#include <vector>
namespace graphics {

struct Vertex {
        ::glm::vec3 position;
        ::glm::vec3 color;
        ::glm::vec3 normal;
        ::glm::vec2 texCoord;
        auto operator==(const Vertex& other) const -> bool;

        static auto getVertexBinding() -> std::vector<render::VertexBinding>;
        static auto getVertexAttribute() -> std::vector<render::VertexAttribute>;
};

}  // namespace graphics

namespace std {
template <>
struct hash<graphics::Vertex> {
        auto operator()(graphics::Vertex const& vertex) const -> size_t {
            return ((hash<glm::vec3>()(vertex.position) ^ (hash<glm::vec3>()(vertex.color) << 1)) >>
                    1) ^
                   (hash<glm::vec2>()(vertex.texCoord) << 1);
        }
};
}  // namespace std