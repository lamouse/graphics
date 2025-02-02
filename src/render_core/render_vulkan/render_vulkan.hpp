#include "render_core/render_base.hpp"
#include "render_core/vulkan_common/device.hpp"
#include "render_core/vulkan_common/memory_allocator.hpp"
#include "render_core/render_vulkan/present_manager.hpp"
#include "render_core/render_vulkan/swapchain.hpp"
#include "render_core/render_vulkan/scheduler.hpp"
#include "render_core/render_vulkan/vk_turbo_mode.hpp"
#include "render_core/render_vulkan/blit_screen.hpp"
#include "render_core/render_vulkan/vk_graphic.hpp"
#include "render_core/render_vulkan/vk_imgui.hpp"

namespace render::vulkan {
auto createDevice(const Instance& instance, vk::SurfaceKHR surface) -> Device;

class RendererVulkan final : public render::RenderBase {
    public:
        CLASS_NON_COPYABLE(RendererVulkan);
        CLASS_NON_MOVEABLE(RendererVulkan);
        explicit RendererVulkan(core::frontend::BaseWindow* window);
        ~RendererVulkan() override;

        [[nodiscard]] auto GetDeviceVendor() const -> std::string override {
            return device.getDriverName();
        }
        void composite(std::span<frame::FramebufferConfig> frame_buffers) override;
        auto getAppletCaptureBuffer() -> std::vector<u8> override;
        auto getGraphics() -> Graphic* override { return &vulkan_graphics; }

    private:
        void RenderAppletCaptureLayer(std::span<const frame::FramebufferConfig> framebuffers);
        void RenderScreenshot(std::span<const frame::FramebufferConfig> framebuffers);
        auto RenderToBuffer(std::span<const frame::FramebufferConfig> framebuffers,
                            const layout::FrameBufferLayout& layout, vk::Format format,
                            vk::DeviceSize buffer_size) -> Buffer;
        Instance instance;
        DebugUtilsMessenger debug_messenger;
        SurfaceKHR surface;
        Device device;
        MemoryAllocator memory_allocator;
        scheduler::Scheduler scheduler;
        Swapchain swapchain;
        PresentManager present_manager;
        BlitScreen blit_swapchain;
        BlitScreen blit_capture;
        std::unique_ptr<Imgui> imgui;
        VulkanGraphics vulkan_graphics;
        std::optional<TurboMode> turbo_mode;
        Frame applet_frame;
};

}  // namespace render::vulkan