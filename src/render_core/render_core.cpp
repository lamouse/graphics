#include "render_core/render_core.hpp"
#include "render_core/render_vulkan/render_vulkan.hpp"
namespace render {
auto createRender(core::frontend::BaseWindow* window) -> std::unique_ptr<RenderBase> {
    return std::make_unique<render::vulkan::RendererVulkan>(window);
}
}  // namespace render