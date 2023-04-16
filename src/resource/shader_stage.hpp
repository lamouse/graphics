#pragma once
#include <vector>

namespace resource::shader {

template<typename T> class ShaderStage
{
    public:
        virtual ::std::vector<T> getShaderStages() = 0;
};

}