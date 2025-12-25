module;
#include "core/frontend/window.hpp"
#include <functional>
#include <atomic>
#include <vulkan/vulkan.hpp>
#include "present_manager.hpp"

export module render.vulkan.ImGui;
import render.vulkan.common;

export namespace render::vulkan {

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