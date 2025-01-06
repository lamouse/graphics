#pragma once
#include "surface.hpp"
#include "types.hpp"
namespace render::texture {
struct ImageInfo {
        ImageInfo() = default;
        surface::PixelFormat format = surface::PixelFormat::Invalid;
        ImageType type = ImageType::e1D;
        SubresourceExtent resources;
        Extent3D size{1, 1, 1};
        union {
                Extent3D block{0, 0, 0};
                u32 pitch;
        };
        u32 layer_stride = 0;
        u32 maybe_unaligned_layer_stride = 0;
        u32 num_samples = 1;
        u32 tile_width_spacing = 0;
        bool rescaleable = false;
        bool downscaleable = false;
        bool forced_flushed = false;
        bool dma_downloaded = false;
        bool is_sparse = false;
};
}  // namespace render::texture