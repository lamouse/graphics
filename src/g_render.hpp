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
        void initPipeline(int width, int height);
        void initLayout();
        ::std::vector<::vk::Fence> fences;
        ::std::vector<::vk::Semaphore> imageAvailableSemaphores;
        ::std::vector<::vk::Semaphore> renderFinshSemaphores;
        void createFances();
        void createsemphores();
        int currentFrame;
        RenderProcess(int width, int height);
        static ::std::unique_ptr<RenderProcess> instance;
    public:
        static RenderProcess& getInstance(){return *instance;}
        static void init(int width, int height);
        static void quit();
        static void reset(int width, int height){quit(); init(width, height);};
        
        void render();
        ~RenderProcess();
    };

}

#endif