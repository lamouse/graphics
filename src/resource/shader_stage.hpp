#pragma once
#include <vector>

namespace resource::shader {

template<typename T> class ShaderStage
{
    public:
        virtual T getShaderStages() = 0;
};

}