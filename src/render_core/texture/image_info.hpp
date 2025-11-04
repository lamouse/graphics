#pragma once
#include "render_core/surface.hpp"
#include "render_core/texture/types.hpp"
namespace render::texture {
struct ImageInfo {
        ImageInfo() = default;
        surface::PixelFormat format = surface::PixelFormat::Invalid;
        SubresourceExtent resources;
        ImageType type = ImageType::e2D;
        Extent3D size{.width = 0, .height = 0, .depth = 1};
        std::uint8_t num_samples = 1;
};
}  // namespace render::texture