module;
#include <cstdint>
export module render.vulkan.common.instance;
import render.vulkan.common.wrapper;
import core;

export namespace render::vulkan {
  [[nodiscard]] auto createInstance(
    std::uint32_t required_version,
    core::frontend::WindowSystemType wst = core::frontend::WindowSystemType::Headless,
    bool enable_validation = false) -> Instance;
}

