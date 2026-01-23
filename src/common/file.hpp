#pragma once
#include <string>
#include <optional>
#include <cstdint>
#include <filesystem>
namespace common {

    enum class FileType{
        Image,
        KtxImage,
        Model,
        UnSupper
    };

    enum class ModuleType{
        Config,
        Model,
        Image,
        Shader,
        Cache,
    };

auto file_hash(const std::string& filepath) -> std::optional<std::uint64_t>;
auto create_dir(const std::string& dirPath) -> bool;
void copy_file(const std::string& src, const std::string& dst);
auto get_current_path() -> std::string;

auto getFileType(const std::string& filepath) -> FileType;
/**
 * @brief if model has mtl file, return mtl path ,else return empty string
 *
 * @param filepath
 * @return std::string
 */
auto model_mtl_file_path(const std::string& filepath) -> std::string;
auto to_string(FileType fileType) -> std::string;
void set_current_path(const std::string& path);
auto get_module_path(ModuleType type) ->std::filesystem::path;


}  // namespace common