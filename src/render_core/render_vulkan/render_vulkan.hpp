#include "render_base.hpp"
#include "vulkan_common/device.hpp"
#include "vulkan_common/memory_allocator.hpp"
#include "present_manager.hpp"
#include "swapchain.hpp"
#include "scheduler.hpp"

namespace render::vulkan {
auto createDevice(const Instance& instance, vk::SurfaceKHR surface) -> Device;

// class RendererVulkan final : public VideoCore::RendererBase {
//     public:
//         explicit RendererVulkan(Core::Frontend::EmuWindow& emu_window,
//                                 Tegra::MaxwellDeviceMemoryManager& device_memory_, Tegra::GPU&
//                                 gpu_, std::unique_ptr<Core::Frontend::GraphicsContext> context_);
//         ~RendererVulkan() override;

//         void Composite(std::span<const Tegra::FramebufferConfig> framebuffers) override;

//         std::vector<u8> GetAppletCaptureBuffer() override;

//         VideoCore::RasterizerInterface* ReadRasterizer() override { return &rasterizer; }

//         [[nodiscard]] std::string GetDeviceVendor() const override {
//             return device.GetDriverName();
//         }

//     private:
//         vk::Buffer RenderToBuffer(std::span<const Tegra::FramebufferConfig> framebuffers,
//                                   const Layout::FramebufferLayout& layout, VkFormat format,
//                                   VkDeviceSize buffer_size);
//         void RenderScreenshot(std::span<const Tegra::FramebufferConfig> framebuffers);
//         void RenderAppletCaptureLayer(std::span<const Tegra::FramebufferConfig> framebuffers);

//         Tegra::MaxwellDeviceMemoryManager& device_memory;
//         Tegra::GPU& gpu;

//         vk::Instance instance;
//         DebugUtilsMessenger debug_messenger;
//         vk::SurfaceKHR surface;

//         Device device;
//         MemoryAllocator memory_allocator;
//         StateTracker state_tracker;
//         scheduler::Scheduler scheduler;
//         Swapchain swapchain;
//         PresentManager present_manager;
//         BlitScreen blit_swapchain;
//         BlitScreen blit_capture;
//         BlitScreen blit_applet;
//         RasterizerVulkan rasterizer;
//         std::optional<TurboMode> turbo_mode;

//         Frame applet_frame;
// };

}  // namespace render::vulkan