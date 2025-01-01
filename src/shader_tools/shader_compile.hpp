#pragma once

#include <string_view>
#include "common/common_funcs.hpp"

namespace shader::compile {
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