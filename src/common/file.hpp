#pragma once
#include <string>
#include <optional>
#include <cstdint>
namespace common {

    enum class FileType{
        Image,
        KtxImage,
        Model,
        UnSupper
    };

auto file_hash(const std::string& filepath) -> std::optional<std::uint64_t>;

auto create_dir(const std::string& dirPath) -> bool;

auto getFileType(const std::string& filepath) -> FileType;
auto to_string(FileType fileType) -> std::string;

}  // namespace common