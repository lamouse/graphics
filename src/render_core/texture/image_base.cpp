
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

ImageBase::ImageBase(const ImageInfo& info_, GPUVAddr gpu_addr_, VAddr cpu_addr_)
    : info{info_},

      has_scaled{},
      gpu_addr{gpu_addr_},
      cpu_addr{cpu_addr_},
      cpu_addr_end{cpu_addr + guest_size_bytes} {
    if (info.type == ImageType::e3D) {
        slice_offsets = {};
        slice_subresources = {};
    }
}

ImageBase::ImageBase(const NullImageParams& /*unused*/) {}

ImageMapView::ImageMapView(GPUVAddr gpu_addr_, VAddr cpu_addr_, size_t size_, ImageId image_id_)
    : gpu_addr{gpu_addr_}, cpu_addr{cpu_addr_}, size{size_}, image_id{image_id_} {}

std::optional<SubresourceBase> ImageBase::TryFindBase(GPUVAddr other_addr) const noexcept {
    if (other_addr < gpu_addr) {
        // Subresource address can't be lower than the base
        return std::nullopt;
    }
    const u32 diff = static_cast<u32>(other_addr - gpu_addr);
    if (diff > guest_size_bytes) {
        // This can happen when two CPU addresses are used for different GPU addresses
        return std::nullopt;
    }
    if (info.type != ImageType::e3D) {
        const auto [layer, mip_offset] = LayerMipOffset(diff, info.layer_stride);
        const auto end = mip_level_offsets.begin() + info.resources.levels;
        const auto it = std::find(mip_level_offsets.begin(), end, static_cast<u32>(mip_offset));
        if (layer > info.resources.layers || it == end) {
            return std::nullopt;
        }
        return SubresourceBase{
            .level = static_cast<s32>(std::distance(mip_level_offsets.begin(), it)),
            .layer = layer,
        };
    } else {
        // TODO: Consider using binary_search after a threshold
        const auto it = std::ranges::find(slice_offsets, diff);
        if (it == slice_offsets.cend()) {
            return std::nullopt;
        }
        return slice_subresources[std::distance(slice_offsets.begin(), it)];
    }
}

ImageViewId ImageBase::FindView(const ImageViewInfo& view_info) const noexcept {
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

bool ImageBase::IsSafeDownload() const noexcept {
    // Skip images that were not modified from the GPU
    if (False(flags & ImageFlagBits::GpuModified)) {
        return false;
    }
    // Skip images that .are. modified from the CPU
    // We don't want to write sensitive data from the guest
    if (True(flags & ImageFlagBits::CpuModified)) {
        return false;
    }
    if (info.num_samples > 1) {
        SPDLOG_WARN("MSAA image downloads are not implemented");
        return false;
    }
    return true;
}

void ImageBase::CheckBadOverlapState() {
    if (False(flags & ImageFlagBits::BadOverlap)) {
        return;
    }
    if (!overlapping_images.empty()) {
        return;
    }
    flags &= ~ImageFlagBits::BadOverlap;
}

void ImageBase::CheckAliasState() {
    if (False(flags & ImageFlagBits::Alias)) {
        return;
    }
    if (!aliased_images.empty()) {
        return;
    }
    flags &= ~ImageFlagBits::Alias;
}

auto AddImageAlias(ImageBase& lhs, ImageBase& rhs, ImageId lhs_id, ImageId rhs_id) -> bool {
    static constexpr auto OPTIONS = RelaxedOptions::Size | RelaxedOptions::Format;
    assert(lhs.info.type == rhs.info.type);
    std::optional<SubresourceBase> base;
    if (lhs.info.type == ImageType::Linear) {
        base = SubresourceBase{.level = 0, .layer = 0};
    } else {
        // We are passing relaxed formats as an option, having broken views/bgr or not won't matter
        static constexpr bool broken_views = false;
        static constexpr bool native_bgr = true;
        base = {};
    }
    if (!base) {
        SPDLOG_ERROR("Image alias should have been flipped");
        return false;
    }
    const surface::PixelFormat lhs_format = lhs.info.format;
    const surface::PixelFormat rhs_format = rhs.info.format;
    const Extent2D lhs_block{
        .width = DefaultBlockWidth(lhs_format),
        .height = DefaultBlockHeight(lhs_format),
    };
    const Extent2D rhs_block{
        .width = DefaultBlockWidth(rhs_format),
        .height = DefaultBlockHeight(rhs_format),
    };
    const bool is_lhs_compressed = lhs_block.width > 1 || lhs_block.height > 1;
    const bool is_rhs_compressed = rhs_block.width > 1 || rhs_block.height > 1;
    const s32 lhs_mips = lhs.info.resources.levels;
    const s32 rhs_mips = rhs.info.resources.levels;
    const s32 num_mips = std::min(lhs_mips - base->level, rhs_mips);
    AliasedImage lhs_alias;
    AliasedImage rhs_alias;
    lhs_alias.id = rhs_id;
    rhs_alias.id = lhs_id;
    lhs_alias.copies.reserve(num_mips);
    rhs_alias.copies.reserve(num_mips);
    for (s32 mip_level = 0; mip_level < num_mips; ++mip_level) {
        Extent3D lhs_size = MipSize(lhs.info.size, base->level + mip_level);
        Extent3D rhs_size = MipSize(rhs.info.size, mip_level);
        if (is_lhs_compressed) {
            lhs_size.width = DivCeil(lhs_size.width, lhs_block.width);
            lhs_size.height = DivCeil(lhs_size.height, lhs_block.height);
        }
        if (is_rhs_compressed) {
            rhs_size.width = DivCeil(rhs_size.width, rhs_block.width);
            rhs_size.height = DivCeil(rhs_size.height, rhs_block.height);
        }
        const Extent3D copy_size{
            .width = std::min(lhs_size.width, rhs_size.width),
            .height = std::min(lhs_size.height, rhs_size.height),
            .depth = std::min(lhs_size.depth, rhs_size.depth),
        };
        if (copy_size.width == 0 || copy_size.height == 0) {
            SPDLOG_WARN("Copy size is smaller than block size. Mip cannot be aliased.");
            continue;
        }
        const bool is_lhs_3d = lhs.info.type == ImageType::e3D;
        const bool is_rhs_3d = rhs.info.type == ImageType::e3D;
        const Offset3D lhs_offset{0, 0, 0};
        const Offset3D rhs_offset{0, 0, is_rhs_3d ? base->layer : 0};
        const s32 lhs_layers = is_lhs_3d ? 1 : lhs.info.resources.layers - base->layer;
        const s32 rhs_layers = is_rhs_3d ? 1 : rhs.info.resources.layers;
        const s32 num_layers = std::min(lhs_layers, rhs_layers);
        const SubresourceLayers lhs_subresource{
            .base_level = mip_level,
            .base_layer = 0,
            .num_layers = num_layers,
        };
        const SubresourceLayers rhs_subresource{
            .base_level = base->level + mip_level,
            .base_layer = is_rhs_3d ? 0 : base->layer,
            .num_layers = num_layers,
        };
        [[maybe_unused]] const ImageCopy& to_lhs_copy = lhs_alias.copies.emplace_back(ImageCopy{
            .src_subresource = lhs_subresource,
            .dst_subresource = rhs_subresource,
            .src_offset = lhs_offset,
            .dst_offset = rhs_offset,
            .extent = copy_size,
        });
        [[maybe_unused]] const ImageCopy& to_rhs_copy = rhs_alias.copies.emplace_back(ImageCopy{
            .src_subresource = rhs_subresource,
            .dst_subresource = lhs_subresource,
            .src_offset = rhs_offset,
            .dst_offset = lhs_offset,
            .extent = copy_size,
        });
        assert(ValidateCopy(to_lhs_copy, lhs.info, rhs.info) && "Invalid RHS to LHS copy");
        assert(ValidateCopy(to_rhs_copy, rhs.info, lhs.info) && "Invalid LHS to RHS copy");
    }
    assert(lhs_alias.copies.empty() == rhs_alias.copies.empty());
    if (lhs_alias.copies.empty()) {
        return false;
    }
    lhs.aliased_images.push_back(std::move(lhs_alias));
    rhs.aliased_images.push_back(std::move(rhs_alias));
    lhs.flags &= ~ImageFlagBits::IsRescalable;
    rhs.flags &= ~ImageFlagBits::IsRescalable;
    return true;
}

}  // namespace render::texture