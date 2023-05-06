#ifndef G_CONTEXT_HPP
#define G_CONTEXT_HPP
#include "core/device.hpp"
#include "system/system_config.hpp"
#include <memory>
#include <functional>
#include <GLFW/glfw3.h>

namespace g{
    
    class Context final
    {
    private:
        Context(const std::vector<const char*>& instanceExtends, core::CreateSurfaceFunc createFunc, bool enableValidationLayers);
        static ::std::unique_ptr<Context> pInstance;
        ::std::shared_ptr<core::Device> device_;

        static int width_;
        static int height_;
        static bool windowIsResize;
    public:
        config::ImageQuality imageQualityConfig;

        auto device()->core::Device&{return  *device_;}
        static void setExtent(int w, int h){width_ = w; height_ = h;}
        static void setWindowResize(){windowIsResize = true;}
        static void resetWindowResize(){windowIsResize = false;}
        static auto isWindowResize() -> bool{ return windowIsResize;}
        static auto getExtent() -> ::vk::Extent2D{return {static_cast<uint32_t>(width_), static_cast<uint32_t>(height_)};}
        static void init(std::vector<const char*>& instanceExtends, core::CreateSurfaceFunc createFunc, int width, int height, bool enableValidationLayers);
        static void quit();
        static void waitWindowEvents(){glfwWaitEvents();}
        static auto Instance() -> Context&;
        ~Context();
    };
    
    
}

#endif