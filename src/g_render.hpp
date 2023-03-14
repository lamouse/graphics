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
        void initRenderPass(::vk::Format format);
        ::std::vector<::vk::Fence> fences;
        ::std::vector<::vk::Semaphore> imageAvailableSemphores;
        ::std::vector<::vk::Semaphore> renderFinshSemphores;
        void createFances();
        void createsemphores();
        int currentFrame;
        RenderProcess(int width, int height, ::vk::Format& format);
        static ::std::unique_ptr<RenderProcess> instance;
    public:
        static RenderProcess& getInstance(){return *instance;}
        static void init(int width, int height, ::vk::Format format);
        static void quit();
        void render();
        ::vk::RenderPass getRenderPass(){return renderPass;};
        ~RenderProcess();
    };

}

#endif