#pragma once

#include <string_view>
#include "common/common_funcs.hpp"
#include <span>
#include "shader_tools/shader_info.h"
namespace shader::compile {

auto getShaderInfo(std::span<const uint32_t> spirv) -> Info;

class ShaderCompile {
    public:
        CLASS_NON_COPYABLE(ShaderCompile);
        CLASS_NON_MOVEABLE(ShaderCompile);
        ShaderCompile();
        ~ShaderCompile();
        void compile(const std::string_view& shader_path, std::string_view out_path);

    private:
};
}  // namespace shader::compile