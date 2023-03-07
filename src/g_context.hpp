#ifndef G_CONTEXT_HPP
#define G_CONTEXT_HPP
#include "g_device.hpp"
#include "g_swapchain.hpp"
#include "g_pipline.hpp"
#include<memory>
#include <functional>

namespace g{
    
    class Context final
    {
    private:
        Context(const std::vector<const char*>& instanceExtends, CreateSurfaceFunc createFunc);
        ::std::shared_ptr<Device> pDevice;

        static ::std::unique_ptr<Context> pInstance;
        vk::Instance createInstance(const std::vector<const char*>& instanceExtends);
        ::std::unique_ptr<Swapchain> pSwapchain;
        Pipline *pipline;
        static int width;
        static int height;
    public:
        static void init(const std::vector<const char*>& instanceExtends, CreateSurfaceFunc createFunc, int width, int height);
        static void quit();
        static Context& getInstance();
        Device& getDevice() noexcept;
        ~Context();
    };
    
    
}

#endif