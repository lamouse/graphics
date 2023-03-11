#include "g_render.hpp"
#include "g_shader.hpp"
#include "g_device.hpp"
#include "g_swapchain.hpp"
#include "g_command.hpp"

namespace g{
::std::unique_ptr<RenderProcess> RenderProcess::instance = nullptr;
RenderProcess::RenderProcess(int width, int height)
{
    initRenderPass();
    initLayout();
    initPipeline(width, height);
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
    Device::getInstance().getVKDevice().destroyRenderPass(renderPass);
    Device::getInstance().getVKDevice().destroyPipelineLayout(layout);
    Device::getInstance().getVKDevice().destroyPipeline(pipline);
}

void RenderProcess::initPipeline(int width, int height)
{
    ::vk::GraphicsPipelineCreateInfo createInfo;
    ::vk::PipelineVertexInputStateCreateInfo inputState;
    createInfo.setPVertexInputState(&inputState);
    ::vk::PipelineInputAssemblyStateCreateInfo inputAsm;
    inputAsm.setPrimitiveRestartEnable(false)
            .setTopology(::vk::PrimitiveTopology::eTriangleList);
    createInfo.setPInputAssemblyState(&inputAsm);

    auto shaderStage = Shader::getInstance().getShaderStage();
    createInfo.setStages(shaderStage);
 
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

void RenderProcess::initRenderPass()
{
    ::vk::RenderPassCreateInfo createInfo;
    ::vk::AttachmentDescription attachDesc;
    attachDesc.setFormat(Swapchain::getInstance().getSwapchainInfo().formatKHR.format)
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
    auto& device = Device::getInstance().getVKDevice();
    ::vk::Semaphore semphore;
    ::vk::SemaphoreCreateInfo semaphoreCreateInfo;
    semphore = device.createSemaphore(semaphoreCreateInfo);
    auto result = device.acquireNextImageKHR(Swapchain::getInstance().getSwapchain(), ::std::numeric_limits<uint16_t>::max(), semphore);
    if(result.result != ::vk::Result::eSuccess)
    {
        throw ::std::runtime_error("acquireNextImageKHR error");
    }
    
    auto imageIndex = result.value;

    Command::getInstance().runCmd(pipline, renderPass, imageIndex);
    device.destroySemaphore(semphore);
}

}