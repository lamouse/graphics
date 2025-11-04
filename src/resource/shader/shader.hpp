#pragma once
#include <cstdint>
#include <vector>
#include <string>
namespace graphics {

auto read_shader(const std::string& shader_name) -> std::vector<std::uint32_t>;
}  // namespace graphics