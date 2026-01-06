#pragma once

#include <cstdint>
#include <string_view>
#include "framebuffer_layout.hpp"
#include "common/class_traits.hpp"
#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif
namespace core::frontend {
/// Information for the Graphics Backends signifying what type of screen pointer is in
/// WindowInformation
enum class WindowSystemType : uint8_t { Headless, Windows, X11, Wayland, Cocoa, Android, Xcb };
class EXPORT BaseWindow {
    public:
        struct Extent {
                int width{};
                int height{};
        };
        struct WindowConfig {
                bool fullscreen = false;
                Extent extent;
        };

        /// Data describing host window system information
        struct WindowSystemInfo {
                // Window system type. Determines which GL context or Vulkan WSI is used.
                WindowSystemType type = WindowSystemType::Headless;

                // Connection to a display server. This is used on X11 and Wayland platforms.
                void* display_connection = nullptr;

                // in vulkan is no null
                void* render_surface = nullptr;

                // Scale of the render surface. For hidpi systems, this will be >1.
                float render_surface_scale = 1.0f;
        };
        void setWindowConfig(const WindowConfig& conf) {
            this->config = conf;
            UpdateCurrentFramebufferLayout(conf.extent.width, conf.extent.height);
        }
        [[nodiscard]] auto getWindowSystemInfo() const -> WindowSystemInfo { return window_info; }
        /**
         * Gets the framebuffer layout (width, height, and screen regions)
         * @note This method is thread-safe
         */
        [[nodiscard]] auto getFramebufferLayout() const -> const layout::FrameBufferLayout& {
            return frame_buffer_layout;
        }
        [[nodiscard]] auto getAspectRatio() const -> float;

        /// Returns if window is shown (not minimized)
        [[nodiscard]] virtual auto IsShown() const -> bool = 0;
        /// Returns if window is minimized
        [[nodiscard]] virtual auto IsMinimized() const -> bool = 0;
        [[nodiscard]] virtual auto shouldClose() const -> bool = 0;
        [[nodiscard]] virtual void setShouldClose() = 0;
        // 这两个主要在imgui使用
        virtual void configGUI() = 0;
        virtual void destroyGUI() = 0;
        virtual void newFrame() = 0;
        [[nodiscard]] virtual auto getActiveConfig() const -> WindowConfig = 0;
        virtual void OnFrameDisplayed() = 0;
        void UpdateCurrentFramebufferLayout(u32 width, u32 height) {
            notifyFramebufferLayoutChanged(layout::DefaultFrameLayout(width, height));
        }
        virtual void pullEvents() = 0;
        virtual void setWindowTitle(std::string_view) = 0;

    protected:
        WindowSystemInfo window_info;
        [[nodiscard]] auto getWindowConfig() const -> WindowConfig { return config; }
        /**
         * Update framebuffer layout with the given parameter.
         * @note EmuWindow implementations will usually use this in window resize event handlers.
         */
        void notifyFramebufferLayoutChanged(const layout::FrameBufferLayout& layout) {
            frame_buffer_layout = layout;
        }

    private:
        WindowConfig config;  ///< Internal configuration (changes pending for being applied in
        ///< Current framebuffer layout
        layout::FrameBufferLayout frame_buffer_layout;

    public:
        BaseWindow() = default;
        CLASS_DEFAULT_COPYABLE(BaseWindow);
        CLASS_DEFAULT_MOVEABLE(BaseWindow);
        virtual ~BaseWindow() = default;
};
}  // namespace core::frontend