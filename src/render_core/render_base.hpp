#pragma once
#include <string>
#include "framebufferConfig.hpp"
#include "core/frontend/window.hpp"
#include "common/common_funcs.hpp"
#include "rasterizer_interface.hpp"

namespace render {
class RenderBase {
    public:
        CLASS_NON_COPYABLE(RenderBase);
        CLASS_NON_MOVEABLE(RenderBase);
        explicit RenderBase(core::frontend::BaseWindow*);
        virtual ~RenderBase() = default;
        [[nodiscard]] virtual auto GetDeviceVendor() const -> std::string = 0;
        virtual auto readRasterizer() -> RasterizerInterface* = 0;
        [[nodiscard]] auto GetRenderWindow(this auto&& self) -> decltype(auto) {
            return std::forward_like<decltype(self)>(self.window_);
        }
        [[nodiscard]] auto getCurrentFPS() const -> float;
        [[nodiscard]] auto getCurrentFrame() const -> int;

    protected:
        core::frontend::BaseWindow* window_;
        float current_fp_ = 0.0f;  ///< Current framerate, should be set by the renderer
        int current_frame_ = 0;    ///< Current frame, should be set by the renderer
    private:
        void UpdateCurrentFramebufferLayout();
};
}  // namespace render