#ifndef G_RENDER_HPP
#define G_RENDER_HPP
#include <vulkan/vulkan.hpp>

namespace g
{
    class RenderProcess
    {
    private:
        ::vk::Pipeline pipline;
        void initPipeline();
    public:
        RenderProcess(/* args */);
        ~RenderProcess();
    };
    
}

#endif