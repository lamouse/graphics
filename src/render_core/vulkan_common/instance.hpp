#pragma once
#include <vulkan/vulkan.hpp>
#include "core/frontend/window.hpp"
namespace render::vulkan {
[[nodiscard]] auto CreateInstance(
    uint32_t required_version,
    core::frontend::WindowSystemType wst = core::frontend::WindowSystemType::Headless,
    bool enable_validation = false) -> vk::Instance;

}