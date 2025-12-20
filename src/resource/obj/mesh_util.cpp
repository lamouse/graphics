#include "resource/obj/mesh_util.hpp"
#include <cstdint>

namespace graphics {
void write_string_vector(std::ostream& os, const std::vector<std::string>& vec) {
    auto size = static_cast<uint32_t>(vec.size());
    os.write(reinterpret_cast<const char*>(&size), sizeof(size));
    for (const auto& str : vec) {
        auto len = static_cast<uint32_t>(str.size());
        os.write(reinterpret_cast<const char*>(&len), sizeof(len));
        if (len > 0) {
            os.write(str.data(), len);
        }
    }
}

auto read_string_vector(std::istream& is, std::vector<std::string>& vec) -> bool {
    uint32_t size = 0;
    if (!is.read(reinterpret_cast<char*>(&size), sizeof(size))) {
        return false;
    }
    vec.resize(size);
    for (auto& str : vec) {
        uint32_t len = 0;
        if (!is.read(reinterpret_cast<char*>(&len), sizeof(len))) {
            return false;
        }
        if (len > 0) {
            str.resize(len);
            if (!is.read(str.data(), len)) {
                return false;
            }
        } else {
            str.clear();
        }
    }
    return true;
};

auto make_vertex_attribute(u32 location, render::VertexAttribute::Type type, u32 offset,
                           render::VertexAttribute::Size size) -> render::VertexAttribute {
    render::VertexAttribute attribute{};
    attribute.hex = 0;
    attribute.location.Assign(location);
    attribute.type.Assign(type);
    attribute.offset.Assign(offset);
    attribute.size.Assign(size);
    return attribute;
}

}  // namespace graphics