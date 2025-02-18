#pragma once
#include <string>
#include <span>
#include <vector>
#include "framebufferConfig.hpp"
#include "core/frontend/window.hpp"
#include "common/common_funcs.hpp"
#include "render_core/graphic.hpp"
#include "render_core/shader_notify.hpp"

namespace render {
class RenderBase {
    public:
        CLASS_NON_COPYABLE(RenderBase);
        CLASS_NON_MOVEABLE(RenderBase);
        explicit RenderBase(core::frontend::BaseWindow*);
        virtual ~RenderBase() = default;
        [[nodiscard]] virtual auto GetDeviceVendor() const -> std::string = 0;
        [[nodiscard]] auto GetRenderWindow(this auto&& self) -> decltype(auto) {
            return std::forward_like<decltype(self)>(self.window_);
        }
        virtual void composite(std::span<frame::FramebufferConfig> frame_buffers) = 0;
        [[nodiscard]] auto getCurrentFPS() const -> float;
        [[nodiscard]] auto getCurrentFrame() const -> int;
        /// Refreshes the settings common to all renderers
        void RefreshBaseSettings();

        virtual auto getAppletCaptureBuffer() -> std::vector<u8> = 0;
        virtual auto getGraphics() -> Graphic* = 0;
        [[nodiscard]] auto getShaderNotify() const -> ShaderNotify& { return *shader_notify_; };

    protected:
        core::frontend::BaseWindow* window_;
        float current_fps_ = 0.0f;  ///< Current framerate, should be set by the renderer
        int current_frame_ = 0;     ///< Current frame, should be set by the renderer
        std::unique_ptr<ShaderNotify> shader_notify_;

    private:
        void UpdateCurrentFramebufferLayout();
};
}  // namespace render