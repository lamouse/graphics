#include "file.hpp"
#include <xxhash.h>
#include <algorithm>
#include <cstddef>
#include <fstream>
#include <vector>
#include <filesystem>
#include <assimp/Importer.hpp>
#include <assimp/DefaultLogger.hpp>
#include <mutex>
namespace common {

namespace {
auto is_model_file(const std::string& filepath) -> bool {
    namespace fs = std::filesystem;
    Assimp::Importer importer;
    auto path = fs::path(filepath);
    if (path.has_extension()) {
        auto extension = path.extension().string();
        if (importer.IsExtensionSupported(path.extension().string())) {
            return true;
        }
    }
    return false;
}

auto is_image_file(const std::string& filepath) {
    namespace fs = std::filesystem;
    Assimp::Importer importer;
    auto path = fs::path(filepath);
    if (path.has_extension()) {
        auto extension = path.extension().string();
        std::ranges::transform(extension, extension.begin(),
                               [](unsigned char c) { return std::tolower(c); });
        if (extension == ".png" || extension == ".jpg") {
            return true;
        }
    }
    return false;
}

auto is_ktx_image_file(const std::string& filepath) {
    namespace fs = std::filesystem;
    Assimp::Importer importer;
    auto path = fs::path(filepath);
    if (path.has_extension()) {
        auto extension = path.extension().string();
        std::ranges::transform(extension, extension.begin(),
                               [](unsigned char c) { return std::tolower(c); });
        if (extension == ".ktx" || extension == ".ktx2") {
            return true;
        }
    }
    return false;
}

std::filesystem::path current_working_path = std::filesystem::current_path();
std::mutex path_mutex;
}  // namespace

auto file_hash(const std::string& filepath) -> std::optional<std::uint64_t> {
    std::ifstream file(filepath, std::ios::binary);
    if (!file) {
        return std::nullopt;
    }
    // Use XXH3 (faster than XXH64 on modern CPUs)
    XXH3_state_t* state = XXH3_createState();
    if (!state) {
        throw std::runtime_error("XXH3_createState failed");
    }

    XXH3_64bits_reset(state);

    constexpr auto BUFFER_SIZE = static_cast<const size_t>(64 * 1024);  // 64KB buffer
    std::vector<char> buffer(BUFFER_SIZE);

    while (file) {
        file.read(buffer.data(), buffer.size());
        std::streamsize bytesRead = file.gcount();
        if (bytesRead > 0) {
            XXH3_64bits_update(state, buffer.data(), static_cast<size_t>(bytesRead));
        }
    }

    uint64_t hash = XXH3_64bits_digest(state);
    XXH3_freeState(state);
    return hash;
}
namespace fs = std::filesystem;
auto create_dir(const std::string& dirPath) -> bool {
    // 检查路径是否已经存在
    if (fs::exists(dirPath)) {
        if (fs::is_directory(dirPath)) {
            return true;
        }
        // 存在但不是目录，可能是文件
        return false;
    }
    // 路径不存在，尝试递归创建
    if (fs::create_directories(dirPath)) {
        return true;
    }
    return false;
}

void copy_file(const std::string& src, const std::string& dst) {
    fs::path src_path(src);
    fs::path dst_path(dst);
    fs::copy(src_path, dst_path, fs::copy_options::update_existing);
}

auto model_mtl_file_path(const std::string& filepath) -> std::string {
    fs::path path(filepath);
    path.replace_extension(".mtl");
    if (fs::exists(path)) {
        return path.string();
    }
    return "";
}

auto get_current_path() -> std::string {
    auto current_path = fs::current_path();
    return current_path.string();
}

auto getFileType(const std::string& filepath) -> FileType {
    if (is_model_file(filepath)) {
        return FileType::Model;
    }
    if (is_image_file(filepath)) {
        return FileType::Image;
    }
    if (is_ktx_image_file(filepath)) {
        return FileType::KtxImage;
    }

    return FileType::UnSupper;
}

auto to_string(FileType fileType) -> std::string {
    switch (fileType) {
        case common::FileType::Model:
            return "model";
        case common::FileType::Image:
            return "image";
        case common::FileType::KtxImage:
            return "ktx image";
        case common::FileType::UnSupper:
            return "Un super";
    }
}

void set_current_path(const std::string& path) {
    std::scoped_lock lock(path_mutex);
    std::filesystem::path new_path(path);
    if (std::filesystem::is_directory(new_path)) {
        current_working_path = std::filesystem::absolute(new_path);
        std::filesystem::current_path(new_path);
    }
}
auto get_module_path(ModuleType type) -> std::filesystem::path {
    std::filesystem::path current_path = current_working_path;
    switch (type) {
        case ModuleType::Config:
            current_path /= "config";
            break;
        case ModuleType::Model:
            current_path /= "models";
            break;
        case ModuleType::Image:
            current_path /= "images";
            break;
        case ModuleType::Shader:
            current_path /= "data/shader";
            break;
        case ModuleType::Cache:
            current_path /= "data/cache";
            break;
    }
    if (!std::filesystem::exists(current_path)) {
        std::filesystem::create_directories(current_path);
    }
    return current_path;
}
}  // namespace common