#pragma once
#include <string>
#include <span>
#include "framebufferConfig.hpp"
#include "core/frontend/window.hpp"
#include "common/common_funcs.hpp"
#include "render_core/graphic.hpp"
#include "render_core/shader_notify.hpp"
#include <functional>

namespace render {
using imgui_ui_fun = std::function<void()>;
class RenderBase {
    public:
        CLASS_NON_COPYABLE(RenderBase);
        CLASS_NON_MOVEABLE(RenderBase);
        explicit RenderBase(core::frontend::BaseWindow*);
        virtual ~RenderBase() = default;
        [[nodiscard]] virtual auto GetDeviceVendor() const -> std::string = 0;
        /// @cond
        /// Get the window associated with this renderer
        // 等待doxygen支持改语法
        [[nodiscard]] auto GetRenderWindow(this auto&& self) -> decltype(auto) {
            return std::forward_like<decltype(self)>(self.window_);
        }
        virtual void composite(std::span<frame::FramebufferConfig> frame_buffers,
                               const imgui_ui_fun& func = nullptr) = 0;
        /// Refreshes the settings common to all renderers
        void RefreshBaseSettings();

        virtual auto getGraphics() -> Graphic* = 0;
        [[nodiscard]] auto getShaderNotify() const -> ShaderNotify& { return *shader_notify_; };

    protected:
        core::frontend::BaseWindow* window_;
        std::unique_ptr<ShaderNotify> shader_notify_;

    private:
        void UpdateCurrentFramebufferLayout();
};
}  // namespace render