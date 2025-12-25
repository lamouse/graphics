module;
#include "core/frontend/window.hpp"
export module render.vulkan.common.instance;
import render.vulkan.common.wrapper;

export namespace render::vulkan {
  [[nodiscard]] auto createInstance(
    uint32_t required_version,
    core::frontend::WindowSystemType wst = core::frontend::WindowSystemType::Headless,
    bool enable_validation = false) -> Instance;
}

