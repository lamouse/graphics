#pragma once
#include "render_core/render_base.hpp"
#include "core/frontend/window.hpp"
#include <memory>
namespace render {
auto createRender(core::frontend::BaseWindow* window) -> std::unique_ptr<RenderBase>;
}