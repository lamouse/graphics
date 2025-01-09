#include "render_core/render_base.hpp"
#include "vulkan_common/device.hpp"
#include "vulkan_common/memory_allocator.hpp"
#include "present_manager.hpp"
#include "swapchain.hpp"
#include "scheduler.hpp"
#include "vk_turbo_mode.hpp"
#include "vk_rasterizer.hpp"
#include "blit_screen.hpp"

namespace render::vulkan {
auto createDevice(const Instance& instance, vk::SurfaceKHR surface) -> Device;

class RendererVulkan final : public render::RenderBase {
    public:
        explicit RendererVulkan(core::frontend::BaseWindow& window);
        ~RendererVulkan() override;

        auto readRasterizer() -> RasterizerInterface* override { return &rasterizer; }

        [[nodiscard]] auto GetDeviceVendor() const -> std::string override {
            return device.getDriverName();
        }
        void composite(std::span<frame::FramebufferConfig> frame_buffers) override;

    private:
        void RenderAppletCaptureLayer(std::span<const frame::FramebufferConfig> framebuffers);
        void RenderScreenshot(std::span<const frame::FramebufferConfig> framebuffers);
        Instance instance;
        DebugUtilsMessenger debug_messenger;
        SurfaceKHR surface;

        Device device;
        MemoryAllocator memory_allocator;
        scheduler::Scheduler scheduler;
        Swapchain swapchain;
        PresentManager present_manager;
        // BlitScreen blit_swapchain;
        // BlitScreen blit_capture;
        // BlitScreen blit_applet;
        BlitScreen blit_swapchain;
        RasterizerVulkan rasterizer;
        std::optional<TurboMode> turbo_mode;

        Frame applet_frame;
};

}  // namespace render::vulkan