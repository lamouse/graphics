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
        RenderProcess(int width, int height);
        static ::std::unique_ptr<RenderProcess> instance;
    public:
        static RenderProcess& getInstance(){return *instance;}
        static void init(int width, int height);
        static void quit();
        void render();
        ::vk::RenderPass getRenderPass(){return renderPass;};
        ~RenderProcess();
    };

}

#endif