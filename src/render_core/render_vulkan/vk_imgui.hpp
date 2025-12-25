//
// Created by ziyu on 2023/11/6:0006.
//

#ifndef GRAPHICS_G_IMGUI_HPP
#define GRAPHICS_G_IMGUI_HPP
#pragma once
#include "core/frontend/window.hpp"
#include <functional>
#include <atomic>
#include "render_core/vulkan_common/vulkan_wrapper.hpp"
import render.vulkan.common;
namespace render::vulkan {
struct Frame;
namespace scheduler {
class Scheduler;
}
class ImguiCore {
    private:
        const Device& device;

        VulkanDescriptorPool descriptorPool;
        core::frontend::BaseWindow* window;
        scheduler::Scheduler& scheduler;
        void newFrame();
        std::atomic_bool is_render_finish;

    public:
        void draw(const std::function<void()>& draw_func, Frame* frame);
        explicit ImguiCore(core::frontend::BaseWindow* window, const Device& device,
                           scheduler::Scheduler& scheduler, vk::PhysicalDevice physical,
                           vk::Instance instance);
        ImguiCore(const ImguiCore&) = delete;
        auto operator=(const ImguiCore&) -> ImguiCore = delete;
        auto operator=(ImguiCore&&) -> ImguiCore = delete;
        ImguiCore(ImguiCore&&) = delete;

        ~ImguiCore();
};

}  // namespace render::vulkan

#endif  // GRAPHICS_G_IMGUI_HPP
