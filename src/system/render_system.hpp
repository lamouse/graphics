#pragma once
#include <vulkan/vulkan.hpp>

namespace engine::system {

class RenderSystem {
    
    public:
        virtual void render() = 0;
};

}