#include "file.hpp"
#include <xxhash.h>
#include <cstddef>
#include <fstream>
#include <vector>
#include <filesystem>
namespace common {

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

}  // namespace common