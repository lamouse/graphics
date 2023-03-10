#ifndef G_RENDER_HPP
#define G_RENDER_HPP
#include <vulkan/vulkan.hpp>

namespace g
{
    class RenderProcess
    {
    private:
        ::vk::Pipeline pipline;
        ::vk::PipelineLayout layout;
        ::vk::RenderPass renderPass;
        void initPipeline(int width, int height);
        void initLayout();
        void initRenderPass();
    public:
        RenderProcess(int width, int height);
        ~RenderProcess();
    };
    
}

#endif