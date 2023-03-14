#include "g_render.hpp"
#include "g_shader.hpp"
#include "g_device.hpp"
#include "g_swapchain.hpp"
#include "g_command.hpp"
#include "g_model.hpp"

namespace g{
::std::unique_ptr<RenderProcess> RenderProcess::instance = nullptr;
RenderProcess::RenderProcess(int width, int height, ::vk::Format& format)
{
    currentFrame = 0;
    initRenderPass(format);
    initLayout();
    initPipeline(width, height);
    createFances();
    createsemphores();
}

void RenderProcess::init(int width, int height, ::vk::Format format)
{
    instance.reset(new RenderProcess(width, height, format));
}

void RenderProcess::quit(){
    instance.reset();
}

RenderProcess::~RenderProcess()
{
    for(auto & fence : fences){
        Device::getInstance().getVKDevice().destroyFence(fence);
    }

    for(auto & semaphore : imageAvailableSemaphores){
        Device::getInstance().getVKDevice().destroySemaphore(semaphore);
    }

        for(auto & semaphore : renderFinshSemaphores){
        Device::getInstance().getVKDevice().destroySemaphore(semaphore);
    }

    Device::getInstance().getVKDevice().destroyRenderPass(renderPass);
    Device::getInstance().getVKDevice().destroyPipelineLayout(layout);
    Device::getInstance().getVKDevice().destroyPipeline(pipline);
}

void RenderProcess::initPipeline(int width, int height)
{
    ::vk::GraphicsPipelineCreateInfo createInfo;
    
    ::vk::PipelineInputAssemblyStateCreateInfo inputAsm;
    inputAsm.setPrimitiveRestartEnable(false)
            .setTopology(::vk::PrimitiveTopology::eTriangleList);
    createInfo.setPInputAssemblyState(&inputAsm);

    auto shaderStage = Shader::getInstance().getShaderStage();
    createInfo.setStages(shaderStage);

    auto bindingDescriptions = Model::Vertex::getBindingDescription();
    auto attributeDescriptions = Model::Vertex::getAtrributeDescription();
    ::vk::PipelineVertexInputStateCreateInfo inputState;
    inputState.setVertexBindingDescriptions(bindingDescriptions);
    inputState.setVertexAttributeDescriptions(attributeDescriptions);
    createInfo.setPVertexInputState(&inputState);    
 
    ::vk::PipelineViewportStateCreateInfo pipelineViewportState;
    ::vk::Viewport viewport(0, 0, width, height, 0, 1);
    pipelineViewportState.setPViewports(&viewport);
    ::vk::Rect2D rect({0, 0}, {static_cast<uint32_t>(width), static_cast<uint32_t>(height)});
    pipelineViewportState.setScissors(rect)
                        .setViewportCount(1)
                        .setScissorCount(1);
    createInfo.setPViewportState(&pipelineViewportState);


    ::vk::PipelineRasterizationStateCreateInfo rastCreateInfo;
    rastCreateInfo.setRasterizerDiscardEnable(false)
                    .setCullMode(::vk::CullModeFlagBits::eBack)
                    .setFrontFace(::vk::FrontFace::eClockwise)
                    .setPolygonMode(::vk::PolygonMode::eFill)
                    .setLineWidth(1);
    createInfo.setPRasterizationState(&rastCreateInfo);

    ::vk::PipelineMultisampleStateCreateInfo multisample;
    multisample.setSampleShadingEnable(false)
                .setRasterizationSamples(::vk::SampleCountFlagBits::e1);
    createInfo.setPMultisampleState(&multisample);

    ::vk::PipelineColorBlendStateCreateInfo blend;
    ::vk::PipelineColorBlendAttachmentState attachs;
    attachs.setBlendEnable(false)
            .setColorWriteMask(::vk::ColorComponentFlagBits::eA | 
                                            ::vk::ColorComponentFlagBits::eB |
                                            ::vk::ColorComponentFlagBits::eG |
                                            ::vk::ColorComponentFlagBits::eR );
    blend.setLogicOpEnable(false)
            .setAttachments(attachs);
    createInfo.setPColorBlendState(&blend);

    createInfo.setLayout(layout)
            .setRenderPass(renderPass);
    auto result = Device::getInstance().getVKDevice().createGraphicsPipeline(nullptr, createInfo);
    if(result.result != vk::Result::eSuccess)
    {
        throw ::std::runtime_error("create graphics pipeline failed");
    }

    pipline = result.value;
}

void RenderProcess::initLayout()
{
    ::vk::PipelineLayoutCreateInfo layoutCrateInfo;
    layout = Device::getInstance().getVKDevice().createPipelineLayout(layoutCrateInfo);
}

void RenderProcess::initRenderPass(::vk::Format format)
{
    ::vk::RenderPassCreateInfo createInfo;
    ::vk::AttachmentDescription attachDesc;
    attachDesc.setFormat(format)
                .setFinalLayout(::vk::ImageLayout::eUndefined)
                .setFinalLayout(::vk::ImageLayout::ePresentSrcKHR)
                .setLoadOp(::vk::AttachmentLoadOp::eClear)
                .setStoreOp(::vk::AttachmentStoreOp::eStore)
                .setStencilLoadOp(::vk::AttachmentLoadOp::eClear)
                .setSamples(::vk::SampleCountFlagBits::e1);
    
    createInfo.setAttachments(attachDesc);

    ::vk::AttachmentReference attachmentReference;
    attachmentReference.setLayout(::vk::ImageLayout::eColorAttachmentOptimal)
                        .setAttachment(0);
    ::vk::SubpassDescription subpass;
    subpass.setPipelineBindPoint(::vk::PipelineBindPoint::eGraphics)
            .setColorAttachments(attachmentReference);
    createInfo.setSubpasses(subpass);

    ::vk::SubpassDependency subepassDependency;
    subepassDependency.setSrcSubpass(VK_SUBPASS_EXTERNAL)
                        .setDstSubpass(0)
                        .setDstAccessMask(::vk::AccessFlagBits::eColorAttachmentWrite)
                        .setSrcStageMask(::vk::PipelineStageFlagBits::eColorAttachmentOutput)
                        .setDstStageMask(::vk::PipelineStageFlagBits::eColorAttachmentOutput);
    createInfo.setDependencies(subepassDependency)
                .setSubpassCount(1);
    renderPass = Device::getInstance().getVKDevice().createRenderPass(createInfo);
}

void RenderProcess::render()
{

    auto result = Device::getInstance().getVKDevice().waitForFences(fences[currentFrame], true, UINT64_MAX);
    if(result != ::vk::Result::eSuccess)
    {
        throw ::std::runtime_error("RenderProcess::render waitForFences error");
    }


    auto& device = Device::getInstance().getVKDevice();
    auto acquireResult = device.acquireNextImageKHR(Swapchain::getInstance().getSwapchain(), ::std::numeric_limits<uint16_t>::max(), imageAvailableSemaphores[currentFrame]);
    if(acquireResult.result == ::vk::Result::eErrorOutOfDateKHR || acquireResult.result != ::vk::Result::eSuccess || acquireResult.result != ::vk::Result::eSuboptimalKHR)
    {
        auto imageIndex = acquireResult.value;
        auto& c =  Swapchain::getInstance();
        auto frameBuffer = Swapchain::getInstance().getFrameBuffer(imageIndex);
        auto extent = Swapchain::getInstance().getSwapchainInfo().extent2D;
        auto swapchain = Swapchain::getInstance().getSwapchain();
        Command::getInstance().runCmd(pipline, renderPass, imageIndex, fences[currentFrame], imageAvailableSemaphores[currentFrame], renderFinshSemaphores[currentFrame], frameBuffer, extent, swapchain);
        currentFrame = (currentFrame + 1) % 2;
    } else {
         throw ::std::runtime_error("Command::getInstance().runCmd error");
    }

    

}

void RenderProcess::createFances()
{
    int count = 2;
    fences.resize(count);
    for(int i = 0; i < count; i++)
    {
        ::vk::FenceCreateInfo info;
        info.setFlags(::vk::FenceCreateFlagBits::eSignaled);
        fences[i] = Device::getInstance().getVKDevice().createFence(info);
    }
}
void RenderProcess::createsemphores()
{
    int count = 2; 
    imageAvailableSemaphores.resize(count);
    for(int i = 0; i < count; i++)
    {
        ::vk::SemaphoreCreateInfo semaphoreCreateInfo;
        imageAvailableSemaphores[i] = Device::getInstance().getVKDevice().createSemaphore(semaphoreCreateInfo);

    }

    renderFinshSemaphores.resize(count);
    for(int i = 0; i < count; i++)
    {
        ::vk::SemaphoreCreateInfo semaphoreCreateInfo;
        renderFinshSemaphores[i] = Device::getInstance().getVKDevice().createSemaphore(semaphoreCreateInfo);

    }
}

}