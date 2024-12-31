#pragma once
#include "surface.hpp"
#include <vulkan/vulkan.hpp>
#include <array>
namespace render::vulkan {
struct RenderPassKey {
        auto operator==(const RenderPassKey&) const noexcept -> bool = default;

        std::array<render::surface::PixelFormat, 8> color_formats;
        render::surface::PixelFormat depth_format;
        vk::SampleCountFlags samples;
};
}  // namespace render::vulkan