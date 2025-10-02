
#include <algorithm>
#include <optional>
#include <utility>
#include <vector>
#include "common/common_types.hpp"
#include "surface.hpp"
#include "image_base.hpp"
#include "image_view_info.hpp"
#include <spdlog/spdlog.h>
namespace {
template <typename N, typename D>
    requires std::is_integral_v<N> && std::is_unsigned_v<D>
[[nodiscard]] constexpr auto DivCeil(N number, D divisor) -> N {
    return static_cast<N>((static_cast<D>(number) + divisor - 1) / divisor);
}

/// Ceiled integer division with logarithmic divisor in base 2
template <typename N, typename D>
    requires std::is_integral_v<N> && std::is_unsigned_v<D>
[[nodiscard]] constexpr auto DivCeilLog2(N value, D alignment_log2) -> N {
    return static_cast<N>((static_cast<D>(value) + (D(1) << alignment_log2) - 1) >> alignment_log2);
}

}  // namespace
namespace render::texture {
using surface::DefaultBlockHeight;
using surface::DefaultBlockWidth;
[[nodiscard]] constexpr u32 AdjustMipSize(u32 size, u32 level) {
    return std::max<u32>(size >> level, 1);
}

[[nodiscard]] constexpr Extent3D AdjustMipSize(Extent3D size, s32 level) {
    return Extent3D{
        .width = AdjustMipSize(size.width, level),
        .height = AdjustMipSize(size.height, level),
        .depth = AdjustMipSize(size.depth, level),
    };
}
Extent3D MipSize(Extent3D size, u32 level) { return AdjustMipSize(size, level); }
/// Returns the base layer and mip level offset
[[nodiscard]] std::pair<s32, s32> LayerMipOffset(s32 diff, u32 layer_stride) {
    if (layer_stride == 0) {
        return {0, diff};
    } else {
        return {diff / layer_stride, diff % layer_stride};
    }
}

[[nodiscard]] bool ValidateLayers(const SubresourceLayers& layers, const ImageInfo& info) {
    return layers.base_level < info.resources.levels &&
           layers.base_layer + layers.num_layers <= info.resources.layers;
}

[[nodiscard]] bool ValidateCopy(const ImageCopy& copy, const ImageInfo& dst, const ImageInfo& src) {
    const Extent3D src_size = MipSize(src.size, copy.src_subresource.base_level);
    const Extent3D dst_size = MipSize(dst.size, copy.dst_subresource.base_level);
    if (!ValidateLayers(copy.src_subresource, src)) {
        return false;
    }
    if (!ValidateLayers(copy.dst_subresource, dst)) {
        return false;
    }
    if (copy.src_offset.x + copy.extent.width > src_size.width ||
        copy.src_offset.y + copy.extent.height > src_size.height ||
        copy.src_offset.z + copy.extent.depth > src_size.depth) {
        return false;
    }
    if (copy.dst_offset.x + copy.extent.width > dst_size.width ||
        copy.dst_offset.y + copy.extent.height > dst_size.height ||
        copy.dst_offset.z + copy.extent.depth > dst_size.depth) {
        return false;
    }
    return true;
}

ImageBase::ImageBase(const ImageInfo& info_) : info{info_} {
    if (info.type == ImageType::e3D) {
        slice_offsets = {};
        slice_subresources = {};
    }
}

ImageBase::ImageBase(const NullImageParams& /*unused*/) {}

ImageMapView::ImageMapView(size_t size_, ImageId image_id_) : size{size_}, image_id{image_id_} {}

auto ImageBase::FindView(const ImageViewInfo& view_info) const noexcept -> ImageViewId {
    const auto it = std::ranges::find(image_view_infos, view_info);
    if (it == image_view_infos.end()) {
        return ImageViewId{};
    }
    return image_view_ids[std::distance(image_view_infos.begin(), it)];
}

void ImageBase::InsertView(const ImageViewInfo& view_info, ImageViewId image_view_id) {
    image_view_infos.push_back(view_info);
    image_view_ids.push_back(image_view_id);
}

auto ImageBase::IsSafeDownload() const noexcept -> bool {
    if (info.num_samples > 1) {
        SPDLOG_WARN("MSAA image downloads are not implemented");
        return false;
    }
    return true;
}

}  // namespace render::texture