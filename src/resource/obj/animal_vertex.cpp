#include "resource/obj/animal_vertex.hpp"
#include "resource/obj/mesh_util.hpp"
#include <cstring>
namespace graphics::animation {
auto Vertex::operator==(const Vertex& other) const -> bool {
    return position == other.position && texCoords == other.texCoords && normal == other.normal &&
           tangent == other.tangent && bitangent == other.bitangent &&
           (std::memcmp(boneIDs, other.boneIDs, sizeof(boneIDs)) == 0);  // NOLINT
}

auto Vertex::getVertexBinding() -> std::vector<render::VertexBinding> {
    std::vector<render::VertexBinding> bindings;
    bindings.push_back(render::VertexBinding{
        .binding = 0, .stride = sizeof(Vertex), .is_instance = false, .divisor = 1});
    return bindings;
}

auto Vertex::getVertexAttribute() -> std::vector<render::VertexAttribute> {
    std::vector<render::VertexAttribute> vertex_attributes;
    u32 location{};
    vertex_attributes.push_back(make_vertex_attribute(
        location++, render::VertexAttribute::Type::Float, offsetof(Vertex, position),
        render::VertexAttribute::Size::R32_G32_B32));

    vertex_attributes.push_back(make_vertex_attribute(
        location++, render::VertexAttribute::Type::Float, offsetof(Vertex, normal),
        render::VertexAttribute::Size::R32_G32_B32));
    vertex_attributes.push_back(
        make_vertex_attribute(location++, render::VertexAttribute::Type::Float,
                              offsetof(Vertex, texCoords), render::VertexAttribute::Size::R32_G32));

    vertex_attributes.push_back(make_vertex_attribute(
        location++, render::VertexAttribute::Type::Float, offsetof(Vertex, tangent),
        render::VertexAttribute::Size::R32_G32_B32));

    vertex_attributes.push_back(make_vertex_attribute(
        location++, render::VertexAttribute::Type::Float, offsetof(Vertex, bitangent),
        render::VertexAttribute::Size::R32_G32_B32));

    vertex_attributes.push_back(make_vertex_attribute(
        location++, render::VertexAttribute::Type::SInt, offsetof(Vertex, boneIDs),
        render::VertexAttribute::Size::R32_G32_B32_A32));

    vertex_attributes.push_back(make_vertex_attribute(
        location++, render::VertexAttribute::Type::Float, offsetof(Vertex, weights),
        render::VertexAttribute::Size::R32_G32_B32_A32));

    return vertex_attributes;
}

}  // namespace graphics::animation