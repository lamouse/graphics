module;
#include "core/frontend/window.hpp"
#include <functional>
#include <atomic>
#include <vulkan/vulkan.hpp>
export module render.vulkan.ImGui;
import render.vulkan.common;
import render.vulkan.scheduler;
import render.vulkan.present.present_frame;

export namespace render::vulkan {

class ImguiCore {
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