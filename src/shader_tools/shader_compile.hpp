#pragma once

#include <string_view>
#include "common/class_traits.hpp"
#include <span>
#include <vector>
#include "shader_tools/shader_info.h"
namespace shader::compile {

auto getShaderInfo(std::span<const uint32_t> spirv) -> Info;

class ShaderCompile {
    public:
        CLASS_NON_COPYABLE(ShaderCompile);
        CLASS_NON_MOVEABLE(ShaderCompile);
        ShaderCompile();
        ~ShaderCompile();
        auto compile(const std::string_view& shader_path) -> std::vector<uint32_t>;

    private:
};
}  // namespace shader::compile