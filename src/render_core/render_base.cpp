#include "render_base.hpp"
#include <cassert>

namespace render {
RenderBase::RenderBase(core::frontend::BaseWindow* window) : window_(window) { assert(window_ != nullptr); }

auto RenderBase::getCurrentFPS() const -> float { return current_fp_; }
auto RenderBase::getCurrentFrame() const -> int { return current_frame_; }
void RenderBase::UpdateCurrentFramebufferLayout() { auto layout = window_->getFramebufferLayout(); }
}  // namespace render