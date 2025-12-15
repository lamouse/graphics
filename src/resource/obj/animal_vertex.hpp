#pragma once
#include <glm/glm.hpp>
#include "render_core/vertex.hpp"
namespace graphics::animation {
constexpr int MAX_BONE_INFLUENCE = 4;
struct Vertex {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 texCoords;
        glm::vec3 tangent;
        glm::vec3 bitangent;
        // bone indexes which will influence this vertex
        int boneIDS[MAX_BONE_INFLUENCE];
        float weights[MAX_BONE_INFLUENCE];
        auto operator==(const Vertex& other) const -> bool;

        static auto getVertexBinding() -> std::vector<render::VertexBinding>;
        static auto getVertexAttribute() -> std::vector<render::VertexAttribute>;
};

}  // namespace graphics::animation