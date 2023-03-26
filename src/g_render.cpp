#include "g_render.hpp"
#include "g_context.hpp"
#include <cassert>
#include <exception>
namespace g{
RenderProcesser::RenderProcesser()
{
    createSwapchain();
    command = ::std::make_unique<Command>(2);
}

RenderProcesser::~RenderProcesser()
{
}

void RenderProcesser::createSwapchain()
{
    auto extent = Context::getExtent();
    while (extent.width == 0 || extent.height == 0)
    {
        extent = Context::getExtent();  
        Context::waitWindowEvents();
    }
    
    Device::getInstance().getVKDevice().waitIdle();
    if(swapchain == nullptr)
    {
        swapchain = ::std::make_unique<Swapchain>(extent.width, extent.height);
    }else{
        ::std::shared_ptr<Swapchain> old = ::std::move(swapchain);
        swapchain = ::std::make_unique<Swapchain>(extent.width, extent.height, old);
        if(!old->compareFormats(*swapchain)){
            throw ::std::runtime_error("swapchain image(or depth) format has changed!");
        }
    }
}

bool RenderProcesser::beginFrame()
{
    auto result  = swapchain->acquireNextImage();

    if(result.result == ::vk::Result::eErrorOutOfDateKHR)
    {
        createSwapchain();
        return isFrameStart;
    }
    if(result.result != ::vk::Result::eSuccess && result.result != ::vk::Result::eSuboptimalKHR)
    {
        throw ::std::runtime_error("faile acquire swapchain image");
    }

    isFrameStart = true;
    currentImageIndex = result.value;
    command->begin(currentFrameIndex);
    return isFrameStart;
}

void RenderProcesser::render(::std::vector<GameObject> gameObjects)
{
}

void RenderProcesser::endFrame()
{
    assert(isFrameStart && "cat't call begin swapchin renderpass is frame not in progress");
    try
    {
        auto result = swapchain->submitCommand(*command, currentImageIndex);

        if(result == ::vk::Result::eErrorOutOfDateKHR || result == ::vk::Result::eSuboptimalKHR || Context::isWindowRsize())
        {
            createSwapchain();
            Context::rsetWindowRsize();
        }else if (result != ::vk::Result::eSuccess)
        {
            throw new ::std::runtime_error("faild to present swap chain image");
        }
    }
    catch(const ::vk::OutOfDateKHRError& e)
    {
        createSwapchain();
        Context::rsetWindowRsize();
    }
    
 
    

    isFrameStart = false;
    currentFrameIndex = (currentFrameIndex + 1) % swapchain->MAX_FRAME_IN_FLIGHT;
}

void RenderProcesser::beginSwapchainRenderPass()
{
    assert(isFrameStart && "cat't call beginSwapchainRenderPass  is frame not in progress");
    swapchain->beginRenderPass(*command, currentFrameIndex);
}

void RenderProcesser::endSwapchainRenderPass()
{
    command->endRenderPass(currentFrameIndex);
}
}
