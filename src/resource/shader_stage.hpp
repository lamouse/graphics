#pragma once
#include <vector>

namespace resource::shader {

template<typename T> class ShaderStage
{
    public:
        virtual auto getShaderStages() -> T = 0;
};

}