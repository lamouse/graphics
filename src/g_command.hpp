#pragma once

#include <vulkan/vulkan.hpp>
#include <memory>
#include <vector>
namespace g
{

class Command final
{
private:
    ::vk::CommandPool cmdPool_;
    ::std::vector<::vk::CommandBuffer> commandBuffers_;
    void initCmdPool();
    void allcoCmdBuffer(int count);
    static ::std::unique_ptr<Command> instance;
    std::vector<::vk::Fence*> imagesInFlight;
    Command(int count);
public:
    static void init(int count);
    static void quit();
    
    ~Command();
    static Command& getInstance(){return *instance;}
    void runCmd(::vk::Pipeline pipline, ::vk::RenderPass renderPass, int index, ::vk::Fence& fence, ::vk::Semaphore& semaphore);
};

}