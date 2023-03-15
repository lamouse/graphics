#include "g_render.hpp"
#include "g_shader.hpp"
#include "g_device.hpp"
#include "g_swapchain.hpp"
#include "g_command.hpp"
#include "g_model.hpp"
#include <iostream>


namespace g{
::std::unique_ptr<RenderProcess> RenderProcess::instance = nullptr;

RenderProcess::RenderProcess(int width, int height)
{
    currentFrame = 0;
    initLayout();
    initPipeline(width, height);
    createFances();
    createsemphores();
}

void RenderProcess::init(int width, int height)
{
    instance.reset(new RenderProcess(width, height));
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
 
    ::vk::Viewport viewport(0, 0, width, height, 0, 1);
    ::vk::Rect2D rect({0, 0}, {static_cast<uint32_t>(width), static_cast<uint32_t>(height)});
    ::vk::PipelineViewportStateCreateInfo pipelineViewportState;
    pipelineViewportState.setViewports(viewport)
                        .setScissors(rect);
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
            .setRenderPass(Swapchain::getInstance().getRenderPass());
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
    ::vk::PushConstantRange pushConstantRange;
    pushConstantRange.setStageFlags(::vk::ShaderStageFlagBits::eVertex | ::vk::ShaderStageFlagBits::eFragment)
                    .setOffset(0)
                    .setSize(sizeof(SimplePushConstantData));
    layoutCrateInfo.setPushConstantRanges(pushConstantRange);
    layout = Device::getInstance().getVKDevice().createPipelineLayout(layoutCrateInfo);
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
    if(acquireResult.result == ::vk::Result::eErrorOutOfDateKHR)
    {
        ::std::cout << "window resize --- " << std::endl;
    }
    if(acquireResult.result == ::vk::Result::eErrorOutOfDateKHR || acquireResult.result != ::vk::Result::eSuccess || acquireResult.result != ::vk::Result::eSuboptimalKHR)
    {
        auto imageIndex = acquireResult.value;
        auto& c =  Swapchain::getInstance();
        auto frameBuffer = Swapchain::getInstance().getFrameBuffer(imageIndex);
        auto extent = Swapchain::getInstance().getSwapchainInfo().extent2D;

        auto swapchain = Swapchain::getInstance().getSwapchain();
        auto renderPass = Swapchain::getInstance().getRenderPass();
        Command::getInstance().begin(imageIndex);
        Command::getInstance().beginRenderPass(imageIndex, pipline, renderPass, extent, frameBuffer);
        Command::getInstance().run(imageIndex, fences[currentFrame], layout, Shader::getInstance().getGameObjects());
        Command::getInstance().endRenderPass(imageIndex);
        Command::getInstance().end(imageIndex, swapchain, imageAvailableSemaphores[currentFrame], renderFinshSemaphores[currentFrame], fences[currentFrame]);
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