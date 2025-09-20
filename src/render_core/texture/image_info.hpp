#pragma once
#include "render_core/surface.hpp"
#include "render_core/texture/types.hpp"
namespace render::texture {
struct ImageInfo {
        ImageInfo() = default;
        surface::PixelFormat format = surface::PixelFormat::Invalid;
        ImageType type = ImageType::e1D;
        SubresourceExtent resources;
        Extent3D size{0, 0, 1};
        union {
                Extent3D block{0, 0, 0};
                u32 pitch;
        } block_or_pitch;
        void* data = nullptr;
        u32 num_samples = 1;

};
}  // namespace render::texture