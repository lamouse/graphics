#include "g_context.hpp"
#include <iostream>
#include<vector>
#include<cassert>
namespace g{


std::unique_ptr<Context> Context::pInstance = nullptr;
int Context::width = 800;
int Context::height = 600;

Context::Context(const std::vector<const char*>& instanceExtends, CreateSurfaceFunc createFunc)
{
    pDevice.reset(new Device(createInstance(instanceExtends), createFunc));
    pSwapchain.reset(new Swapchain(pDevice, width, height));
}

Context::~Context()
{
    pSwapchain.reset();
    pDevice.reset();
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

Context& Context::getInstance()
{   
    assert(pInstance);
    return *pInstance;
}

vk::Instance Context::createInstance(const std::vector<const char*>& instanceExtends)
{
    ::std::vector<const char*> layers{"VK_LAYER_KHRONOS_validation"};
    ::std::vector<const char*> externs = {"VK_KHR_portability_enumeration"};
    externs.insert(externs.end(), instanceExtends.begin(), instanceExtends.end());
    ::vk::InstanceCreateFlags flags{VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR};
    ::vk::InstanceCreateInfo createInfo;
    ::vk::ApplicationInfo appInfo;
     appInfo.setApiVersion(VK_API_VERSION_1_3);
     createInfo.setPEnabledLayerNames(layers)
                .setPApplicationInfo(&appInfo)
                .setFlags(flags)
                .setPEnabledExtensionNames(externs);
    
    return ::vk::createInstance(createInfo);
}

Device& Context::getDevice()noexcept
{
    return *pDevice;
}

}

