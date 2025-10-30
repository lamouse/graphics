#include "graphics/graphic.hpp"
#include "graphics/sdl_window.hpp"
#include "graphics/glfw_window.hpp"
#include "graphics/gui.hpp"
#include "render_core/render_vulkan/render_vulkan.hpp"

namespace graphics {
auto createWindow() -> std::unique_ptr<core::frontend::BaseWindow> {
    const int width = 1920;
    const int height = 1080;
    const char* title = "graphic engine";

#if defined(USE_GLFW)
    return std::make_unique<ScreenWindow>(width, height, title);
#endif
#if defined(USE_SDL)
    return std::make_unique<graphics::SDLWindow>(width, height, title);
#endif
    return nullptr;
}

auto createRender(core::frontend::BaseWindow* window) -> std::unique_ptr<render::RenderBase>{
    ui::init_imgui(window->getWindowSystemInfo().render_surface_scale);
    return std::make_unique<render::vulkan::RendererVulkan>(window);
}
}  // namespace graphics