module;
#include "render_core/render_base.hpp"
export module render.base;
export namespace render {
using imgui_ui_fun = std::function<void()>;
using RenderBase = render::RenderBase;
}