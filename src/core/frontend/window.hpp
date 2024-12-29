#pragma once

#include <cstdint>
#include <functional>
#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif
namespace vk {
class Instance;
class SurfaceKHR;
}  // namespace vk
namespace core::frontend {
/// Information for the Graphics Backends signifying what type of screen pointer is in
/// WindowInformation
enum class WindowSystemType : uint8_t {
    Headless,
    Windows,
    X11,
    Wayland,
    Cocoa,
    Android,
};
class EXPORT BaseWindow {
    public:
        struct Extent {
                int width;
                int height;
        };
        struct WindowConfig {
                bool fullscreen = false;
                Extent extent = {.width = 0, .height = 0};
        };
        using get_surface_fn = std::function<vk::SurfaceKHR(vk::Instance)>;
        /// Data describing host window system information
        struct WindowSystemInfo {
                // Window system type. Determines which GL context or Vulkan WSI is used.
                WindowSystemType type = WindowSystemType::Headless;

                // Connection to a display server. This is used on X11 and Wayland platforms.
                void* display_connection = nullptr;

                // in vulkan is no null
                void* get_surface = nullptr;

                // Scale of the render surface. For hidpi systems, this will be >1.
                float render_surface_scale = 1.0f;
        };
        void setWindowConfig(const WindowConfig& config) { this->config = config; }
        [[nodiscard]] auto getWindowSystemInfo() const -> WindowSystemInfo { return window_info; }
        /// Returns if window is shown (not minimized)
        [[nodiscard]] virtual auto IsShown() const -> bool = 0;
        /// Returns if window is minimized
        [[nodiscard]] virtual auto IsMinimized() const -> bool = 0;
        [[nodiscard]] virtual auto shouldClose() const -> bool = 0;
        [[nodiscard]] virtual auto getActiveConfig() const -> WindowConfig = 0;

    protected:
        WindowSystemInfo window_info;
        [[nodiscard]] auto getWindowConfig() const -> WindowConfig { return config; }

    private:
        WindowConfig config;  ///< Internal configuration (changes pending for being applied in ///
                              ///< ProcessConfigurationChanges)
    public:
        BaseWindow() = default;
        BaseWindow(const BaseWindow&) = default;
        BaseWindow(BaseWindow&&) = default;
        auto operator=(const BaseWindow&) -> BaseWindow& = default;
        auto operator=(BaseWindow&&) -> BaseWindow& = default;
        virtual ~BaseWindow() = default;
};
}  // namespace core::frontend