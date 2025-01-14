#pragma once
#include "render_core/surface.hpp"
#include "render_core/texture/types.hpp"
#include "common/common_funcs.hpp"
namespace render::texture {
struct ImageViewInfo;
struct ImageInfo;

struct NullImageViewParams {};

enum class ImageViewFlagBits : u16 {
    PreemtiveDownload = 1 << 0,
    Strong = 1 << 1,
    Slice = 1 << 2,
};
DECLARE_ENUM_FLAG_OPERATORS(ImageViewFlagBits)

struct ImageViewBase {
        explicit ImageViewBase(const ImageViewInfo& info, const ImageInfo& image_info,
                               ImageId image_id, GPUVAddr addr);
        explicit ImageViewBase(const ImageInfo& info, const ImageViewInfo& view_info,
                               GPUVAddr addr);
        explicit ImageViewBase(const NullImageViewParams&);

        [[nodiscard]] bool IsBuffer() const noexcept { return type == ImageViewType::Buffer; }

        [[nodiscard]] bool SupportsAnisotropy() const noexcept;

        ImageId image_id{};
        GPUVAddr gpu_addr = 0;
        surface::PixelFormat format{};
        ImageViewType type{};
        SubresourceRange range;
        Extent3D size{0, 0, 0};
        ImageViewFlagBits flags{};

        u64 invalidation_tick = 0;
        u64 modification_tick = 0;
};
}  // namespace render::texture