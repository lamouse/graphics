#include "resource/obj/mesh_vertex.hpp"
namespace graphics {
auto Vertex::getVertexBinding() -> std::vector<render::VertexBinding> {
    std::vector<render::VertexBinding> bindings;
    bindings.push_back(render::VertexBinding{
        .binding = 0, .stride = sizeof(Vertex), .is_instance = false, .divisor = 1});
    return bindings;
}

auto Vertex::getVertexAttribute() -> std::vector<render::VertexAttribute> {
    std::vector<render::VertexAttribute> vertex_attributes;

    render::VertexAttribute position{};
    position.hex = 0;
    position.location.Assign(0);
    position.type.Assign(render::VertexAttribute::Type::Float);
    position.offset.Assign(offsetof(Vertex, position));
    position.size.Assign(render::VertexAttribute::Size::R32_G32_B32);
    vertex_attributes.push_back(position);

    render::VertexAttribute color{};
    color.hex = 0;
    color.location.Assign(1);
    color.type.Assign(render::VertexAttribute::Type::Float);
    color.offset.Assign(offsetof(Vertex, color));
    color.size.Assign(render::VertexAttribute::Size::R32_G32_B32);
    vertex_attributes.push_back(color);

    render::VertexAttribute normal{};
    normal.hex = 0;
    normal.location.Assign(2);
    normal.type.Assign(render::VertexAttribute::Type::Float);
    normal.offset.Assign(offsetof(Vertex, normal));
    normal.size.Assign(render::VertexAttribute::Size::R32_G32_B32);
    vertex_attributes.push_back(normal);

    render::VertexAttribute texCoord{};
    texCoord.hex = 0;
    texCoord.location.Assign(3);
    texCoord.type.Assign(render::VertexAttribute::Type::Float);
    texCoord.offset.Assign(offsetof(Vertex, texCoord));
    texCoord.size.Assign(render::VertexAttribute::Size::R32_G32);
    vertex_attributes.push_back(texCoord);
    return vertex_attributes;
}

auto Vertex::operator==(const Vertex& other) const -> bool {
    return position == other.position && color == other.color && normal == other.normal &&
           texCoord == other.texCoord;
}

}  // namespace graphics