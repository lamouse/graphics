#include "resource/obj/mesh_vertex.hpp"
#include "resource/obj/mesh_util.hpp"
namespace graphics {
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
    vertex_attributes.push_back(
        make_vertex_attribute(location++, render::VertexAttribute::Type::Float,
                              offsetof(Vertex, color), render::VertexAttribute::Size::R32_G32_B32));
    vertex_attributes.push_back(make_vertex_attribute(
        location++, render::VertexAttribute::Type::Float, offsetof(Vertex, normal),
        render::VertexAttribute::Size::R32_G32_B32));
    vertex_attributes.push_back(
        make_vertex_attribute(location++, render::VertexAttribute::Type::Float,
                              offsetof(Vertex, texCoord), render::VertexAttribute::Size::R32_G32));

    return vertex_attributes;
}

auto Vertex::operator==(const Vertex& other) const -> bool {
    return position == other.position && color == other.color && normal == other.normal &&
           texCoord == other.texCoord;
}

}  // namespace graphics