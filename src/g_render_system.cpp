#include "g_render_system.hpp"
#include "g_device.hpp"
#include <memory>
namespace g
{

void RenderSystem::renderGameObject(::std::vector<GameObject>& gameObjects, const Camera& camera, ::vk::CommandBuffer commandBuffer)
{
    pipeline->bind(commandBuffer);
    auto projectionView = camera.getProjection() * camera.getInverseView();

    for(auto& obj : gameObjects)
    {
        SimplePushConstantData push;
        push.color = obj.color;
        //obj.transform.rotation.y = ::glm::mod(obj.transform.rotation.y + 0.0003f, ::glm::two_pi<float>());
        //obj.transform.rotation.x = ::glm::mod(obj.transform.rotation.x + 0.0001f, ::glm::two_pi<float>());
        push.transform = obj.transform.mat4();
        // commandBuffer.pushConstants(pipelineLayout, ::vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 
        //                         0, sizeof(SimplePushConstantData), &push);
        obj.model->bind(commandBuffer);
        obj.model->draw(commandBuffer);
    }
}
    
RenderSystem::RenderSystem(::vk::RenderPass renderPass)
{
    createPipelineLayout();
    createPipeline(renderPass);
}

RenderSystem::~RenderSystem()
{
    Device::getInstance().getVKDevice().destroyPipelineLayout(pipelineLayout);
}

void RenderSystem::createPipelineLayout()
{
    ::vk::PushConstantRange pushConstantRange;
    pushConstantRange.setStageFlags(::vk::ShaderStageFlagBits::eVertex | 
                                    ::vk::ShaderStageFlagBits::eFragment)
                        .setOffset(0)
                        .setSize(sizeof(SimplePushConstantData));
    ::vk::PipelineLayoutCreateInfo layoutCreateInfo;
    //layoutCreateInfo.setPushConstantRanges(pushConstantRange);  
    pipelineLayout = Device::getInstance().getVKDevice().createPipelineLayout(layoutCreateInfo); 
}

void RenderSystem::createPipeline(::vk::RenderPass renderPass)
{
#if defined(VK_USE_PLATFORM_MACOS_MVK)
    ::std::string full_path{"/Users/sora/project/cpp/test/xmake/graphics/src/shader/"};
#else
    ::std::string full_path{"E:/project/cpp/graphics/src/shader/"};
#endif
    pipeline = ::std::make_unique<PipeLine>(full_path + "vert.spv", full_path + "frag.spv", renderPass, pipelineLayout);
}

}
