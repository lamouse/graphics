#ifndef G_RENDER_HPP
#define G_RENDER_HPP
#include <vulkan/vulkan.hpp>

namespace g
{
    class RenderProcess
    {
    private:
        ::vk::Pipeline pipline;
        void initPipeline(int width, int height);
    public:
        RenderProcess(int width, int height);
        ~RenderProcess();
    };
    
}

#endif