#pragma once
#include <string>
#include <optional>
#include <cstdint>
namespace common {

auto file_hash(const std::string& filepath) -> std::optional<std::uint64_t>;

auto create_dir(const std::string& dirPath) -> bool;

}  // namespace common