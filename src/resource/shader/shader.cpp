#include "shader/shader.hpp"
#include <stdexcept>
#include <filesystem>
#include <fstream>
#include <cstring>
#include "source_shader.h"
namespace graphics {
auto read_shader(const std::string& shader_name) -> std::vector<std::uint32_t> {
    namespace fs = std::filesystem;

    const std::string& filepath = shader::SHADER_ROOT_PATH + shader_name + shader::SHADER_EXTENSION;

    // 检查文件是否存在
    if (!fs::exists(filepath)) {
        throw std::runtime_error("Shader file not found: " + filepath);
    }

    // 检查是否为普通文件
    if (!fs::is_regular_file(filepath)) {
        throw std::runtime_error("Not a valid file: " + filepath);
    }

    // 打开文件为二进制模式
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open shader file: " + filepath);
    }

    // 获取文件大小
    file.seekg(0, std::ios::end);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    // SPIR-V 是以 uint32_t 为单位的，所以大小必须是 4 的倍数
    if (size % 4 != 0) {
        throw std::runtime_error("Invalid SPIR-V file: size not a multiple of 4");
    }

    // 分配缓冲区（以字节为单位）
    std::vector<char> buffer(size);
    if (!file.read(buffer.data(), size)) {
        throw std::runtime_error("Failed to read shader file: " + filepath);
    }
    file.close();

    // 将字节转换为 uint32_t 向量
    // 注意：SPIR-V 默认是小端序（Little Endian）
    std::vector<std::uint32_t> spirv;
    spirv.resize(size / 4);

    // 将 char[] 按 4 字节拷贝到 uint32_t[]
    std::memcpy(spirv.data(), buffer.data(), size);

    return spirv;
}
}  // namespace graphics