#pragma once

#include <vulkan/vulkan.hpp>
#include <memory>
namespace g
{

class Command final
{
private:
    ::vk::CommandPool cmdPool_;
    ::vk::CommandBuffer cmdBuffer_;
    ::vk::Fence fence;
    void initCmdPool();
    void allcoCmdBuffer();
    void createFance();
    static ::std::unique_ptr<Command> instance;
    Command();

public:
    static void init();
    static void quit();
    ~Command();
    static Command& getInstance(){return *instance;}
    void runCmd(::vk::Pipeline pipline, ::vk::RenderPass renderPass, int index);
    ::vk::CommandBuffer getCmdBuffer(){return cmdBuffer_;}
};

}