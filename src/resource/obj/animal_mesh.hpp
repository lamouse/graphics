#pragma once
#include "resource/obj/animal_vertex.hpp"
#include "resource/obj/mesh_material.hpp"
#include <vector>
namespace graphics::animation {

class Mesh {
    public:
        std::vector<Vertex> vertices_;
        std::vector<glm::vec3> positions_;
        std::vector<std::uint32_t> indices_;
        MeshMaterial material_;
        Mesh(const std::vector<Vertex>& vertices, const std::vector<glm::vec3>& positions,
             const std::vector<std::uint32_t>& indices, MeshMaterial& material)
            : vertices_(vertices), positions_(positions), indices_(indices), material_(material) {}
};

}  // namespace graphics::animation