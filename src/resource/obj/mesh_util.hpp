#pragma once
#include <iostream>
#include <vector>
#include "render_core/vertex.hpp"
namespace graphics {
void write_string_vector(std::ostream& os, const std::vector<std::string>& vec);
auto read_string_vector(std::istream& is, std::vector<std::string>& vec) -> bool;
auto make_vertex_attribute(u32 location, render::VertexAttribute::Type type, u32 offset,
                           render::VertexAttribute::Size size) -> render::VertexAttribute;
}  // namespace graphics