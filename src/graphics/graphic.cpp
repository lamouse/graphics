#include "graphics/graphic.hpp"
#if defined(USE_SDL)
#include "graphics/sdl_window.hpp"
#elif defined(USE_QT)
#include "graphics/QT_window.hpp"
#else
#error "Must define either USE_SDL or USE_QT"
#endif
#include "graphics/gui.hpp"
#include "render_core/render_core.hpp"

namespace graphics {
auto createWindow() -> std::unique_ptr<core::frontend::BaseWindow> {
    const int width = 1920;
    const int height = 1080;
    const char* title = "graphic engine";

#if defined(USE_SDL)
    return std::make_unique<graphics::SDLWindow>(width, height, title);
#elif defined(USE_QT)
    return std::make_unique<graphics::QTWindow>(width, height, title);
#endif
    return nullptr;
}

auto createRender(core::frontend::BaseWindow* window) -> std::unique_ptr<render::RenderBase> {
    ui::init_imgui(window->getWindowSystemInfo().render_surface_scale);
    return render::createRender(window);
}
}  // namespace graphics