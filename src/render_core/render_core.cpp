#include "render_core/render_core.hpp"
import render.impl.vulkan;
namespace render {
auto createRender(core::frontend::BaseWindow* window) -> std::unique_ptr<RenderBase> {
    return std::make_unique<render::vulkan::RendererVulkan>(window);
}
}  // namespace render