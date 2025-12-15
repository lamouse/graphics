#pragma once
#include <iostream>
#include <vector>
namespace graphics {
void write_string_vector(std::ostream& os, const std::vector<std::string>& vec);
auto read_string_vector(std::istream& is, std::vector<std::string>& vec) -> bool;
}  // namespace graphics