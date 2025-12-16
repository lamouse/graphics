#pragma once
#include <glm/glm.hpp>
#include "render_core/vertex.hpp"
namespace graphics::animation {
constexpr int MAX_BONE_INFLUENCE = 4;
struct Vertex {
        glm::vec3 position{};
        glm::vec3 normal{};
        glm::vec2 texCoords{};
        glm::vec3 tangent{};
        glm::vec3 bitangent{};
        // bone indexes which will influence this vertex
        int boneIDs[MAX_BONE_INFLUENCE]{};
        float weights[MAX_BONE_INFLUENCE]{};
        auto operator==(const Vertex& other) const -> bool;

        static auto getVertexBinding() -> std::vector<render::VertexBinding>;
        static auto getVertexAttribute() -> std::vector<render::VertexAttribute>;
        Vertex() {
            for (int i = 0; i < MAX_BONE_INFLUENCE; i++) {
                boneIDs[i] = -1;
                weights[i] = 0.0f;
            }
        }
        void setBone(int boneID, float weight) {
            for (int i = 0; i < MAX_BONE_INFLUENCE; i++) {
                if (boneIDs[i] < 0) {
                    boneIDs[i] = boneID;
                    weights[i] = weight;
                    break;
                }
            }
        }
};

}  // namespace graphics::animation