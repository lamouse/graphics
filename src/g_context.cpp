#include "g_context.hpp"
#include <iostream>
#include<vector>
#include<cassert>
namespace g{


::std::unique_ptr<Context> Context::pInstance = nullptr;
int Context::width = 800;
int Context::height = 600;

Context::Context(const std::vector<const char*>& instanceExtends, CreateSurfaceFunc createFunc)
{
    auto vkInstance = createInstance(instanceExtends);
    Device::init(vkInstance, createFunc(vkInstance));

#if defined(VK_USE_PLATFORM_MACOS_MVK)
    ::std::string full_path{"/Users/sora/project/cpp/test/xmake/graphics/src/shader/"};
#else
    ::std::string full_path{"E:/project/cpp/graphics/src/shader/"};
#endif
    
    Shader::init(full_path + "vert.spv", 
            full_path + "frag.spv");
    Swapchain::init(width, height);
    RenderProcess::init(width, height, Swapchain::getInstance().getSwapchainInfo().formatKHR.format);
    Swapchain::getInstance().createImageFrame();
    Command::init(Swapchain::getInstance().getImageCount());
    Shader::getInstance().loadModel();

    
}

Context::~Context()
{
    Device::getInstance().getVKDevice().waitIdle();
    Command::quit();
    RenderProcess::quit();
    Swapchain::quit();
    Shader::quit();
    Device::quit();
}

void Context::init(const std::vector<const char*>& instanceExtends, CreateSurfaceFunc createFunc, int width, int height)
{
    width = width;
    height = height;
    pInstance.reset(new Context(instanceExtends, createFunc));
}
void Context::quit()
{
    pInstance.reset();
}

Context& Context::Instance()
{   
    assert(pInstance);
    return *pInstance;
}

::vk::Instance Context::createInstance(const std::vector<const char*>& instanceExtends)
{
    ::std::vector<const char*> layers{"VK_LAYER_KHRONOS_validation"};
    ::std::vector<const char*> externs = {instanceExtends.begin(), instanceExtends.end()};
#if defined(VK_USE_PLATFORM_MACOS_MVK)
    externs.push_back("VK_KHR_portability_enumeration");
    ::vk::InstanceCreateFlags flags{VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR};
#endif
 
    ::vk::InstanceCreateInfo createInfo;
    ::vk::ApplicationInfo appInfo;
     appInfo.setApiVersion(VK_API_VERSION_1_3);
     createInfo.setPEnabledLayerNames(layers)
                .setPApplicationInfo(&appInfo)
#if defined(VK_USE_PLATFORM_MACOS_MVK)
                .setFlags(flags)
#endif
                .setPEnabledExtensionNames(externs);
    return ::vk::createInstance(createInfo);
}

}

