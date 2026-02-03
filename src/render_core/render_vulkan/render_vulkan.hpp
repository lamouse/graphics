#include "render_core/render_base.hpp"
#include "render_core/vulkan_common/device.hpp"
#include "render_core/vulkan_common/memory_allocator.hpp"
#include "render_core/render_vulkan/present_manager.hpp"
#include "render_core/render_vulkan/swapchain.hpp"
#include "render_core/render_vulkan/scheduler.hpp"
#include "render_core/render_vulkan/blit_screen.hpp"
#include "render_core/render_vulkan/vk_graphic.hpp"

namespace render::vulkan {
auto createDevice(const Instance& instance, vk::SurfaceKHR surface) -> Device;

class RendererVulkan final : public render::RenderBase {
    public:
        RendererVulkan(const RendererVulkan&) = delete;
        RendererVulkan(RendererVulkan&&)noexcept = delete;
        auto operator=(const RendererVulkan&) ->RendererVulkan& = delete;
        auto operator=(RendererVulkan&&) noexcept -> RendererVulkan& = delete;
        explicit RendererVulkan(core::frontend::BaseWindow* window);
        ~RendererVulkan() override;

        [[nodiscard]] auto GetDeviceVendor() const -> std::string override {
            return device.getDriverName();
        }
        void composite(std::span<frame::FramebufferConfig> frame_buffers,
                       const imgui_ui_fun& func = nullptr) override;
        /// @cond
        // TODO doxygen seems to have problems with this
        auto getGraphics() -> Graphic* override { return &vulkan_graphics; }
        /// @endcond

    private:
        Instance instance;
        DebugUtilsMessenger debug_messenger;
        SurfaceKHR surface;
        Device device;
        MemoryAllocator memory_allocator;
        scheduler::Scheduler scheduler;
        Swapchain swapchain;
        PresentManager present_manager;
        BlitScreen blit_swapchain;
        std::unique_ptr<ImguiCore> imgui;
        VulkanGraphics vulkan_graphics;
};

}  // namespace render::vulkan