#include "render_base.hpp"
#include <cassert>

namespace render {
RenderBase::RenderBase(core::frontend::BaseWindow* window) : window_(window) {
    assert(window_ != nullptr);
    shader_notify_ = std::make_unique<ShaderNotify>();
    RefreshBaseSettings();
}

void RenderBase::RefreshBaseSettings() {
    UpdateCurrentFramebufferLayout();
}


auto RenderBase::getCurrentFPS() const -> float { return current_fps_; }
auto RenderBase::getCurrentFrame() const -> int { return current_frame_; }
void RenderBase::UpdateCurrentFramebufferLayout() { auto layout = window_->getFramebufferLayout();
    window_->UpdateCurrentFramebufferLayout(layout.width, layout.height);
 }
}  // namespace render