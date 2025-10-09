//
// Created by ziyu on 2023/11/6:0006.
//

#ifndef GRAPHICS_G_IMGUI_HPP
#define GRAPHICS_G_IMGUI_HPP
#pragma once
#include "render_core/vulkan_common/device.hpp"
#include "core/frontend/window.hpp"
namespace render::vulkan {
class ImguiCore {
    private:
        RenderPass render_pass;
        void init_debug_info();
        VulkanDescriptorPool descriptorPool;
        core::frontend::BaseWindow* window;

    public:
        void draw(const vk::CommandBuffer& commandBuffer);
        void imgui_predraw();
        explicit ImguiCore(core::frontend::BaseWindow* window, const Device& device,
                           vk::PhysicalDevice physical, vk::Instance instance, float scale = 1.0f);
        ImguiCore(const ImguiCore&) = delete;
        auto operator=(const ImguiCore&) -> ImguiCore = delete;
        auto operator=(ImguiCore&&) -> ImguiCore = delete;
        ImguiCore(ImguiCore&&) = delete;
        void newFrame();
        void endFrame();
        ~ImguiCore();
};

}  // namespace render::vulkan

#endif  // GRAPHICS_G_IMGUI_HPP
