#pragma once

#include <array>
#include <optional>
#include <vector>
#include <boost/container/small_vector.hpp>

#include "common/common_funcs.hpp"
#include "common/common_types.hpp"
#include "render_core/texture/image_info.hpp"
#include "render_core/texture/image_view_info.hpp"
#include "render_core/texture/types.hpp"

namespace render::texture {
enum class ImageFlagBits : u32 {
    AcceleratedUpload = 1 << 0,  ///< Upload can be accelerated in the GPU
    Converted = 1 << 1,    ///< Guest format is not supported natively and it has to be converted
    CpuModified = 1 << 2,  ///< Contents have been modified from the CPU
    GpuModified = 1 << 3,  ///< Contents have been modified from the GPU
    Tracked = 1 << 4,      ///< Writes and reads are being hooked from the CPU JIT
    Strong = 1 << 5,       ///< Exists in the image table, the dimensions are can be trusted
    Registered = 1 << 6,   ///< True when the image is registered
    Picked = 1 << 7,       ///< Temporary flag to mark the image as picked
    Remapped = 1 << 8,     ///< Image has been remapped.
    Sparse = 1 << 9,       ///< Image has non continuous submemory.

    // Garbage Collection Flags
    BadOverlap = 1 << 10,  ///< This image overlaps other but doesn't fit, has higher
                           ///< garbage collection priority
    Alias = 1 << 11,       ///< This image has aliases and has priority on garbage
                           ///< collection
    CostlyLoad = 1 << 12,  ///< Protected from low-tier GC as it is costly to load back.

    // Rescaler
    Rescaled = 1 << 13,
    CheckingRescalable = 1 << 14,
    IsRescalable = 1 << 15,

    AsynchronousDecode = 1 << 16,
    IsDecoding = 1 << 17,  ///< Is currently being decoded asynchronously.
};
DECLARE_ENUM_FLAG_OPERATORS(ImageFlagBits)

struct ImageViewInfo;
struct AliasedImage {
        std::vector<ImageCopy> copies;
        ImageId id;
};
struct NullImageParams {};

struct ImageBase {
        explicit ImageBase(const ImageInfo& info);
        explicit ImageBase(const NullImageParams&);

        [[nodiscard]] auto FindView(const ImageViewInfo& view_info) const noexcept -> ImageViewId;

        void InsertView(const ImageViewInfo& view_info, ImageViewId image_view_id);

        [[nodiscard]] auto IsSafeDownload() const noexcept -> bool;

        void CheckBadOverlapState();
        void CheckAliasState();

        [[nodiscard]] auto HasScaled() const -> bool { return has_scaled; }

        ImageInfo info;

        u32 guest_size_bytes = 0;
        u32 converted_size_bytes = 0;
        u32 scale_rating = 0;
        u64 scale_tick = 0;
        bool has_scaled = false;

        size_t channel = 0;

        ImageFlagBits flags = ImageFlagBits::CpuModified;

        u64 modification_tick = 0;
        size_t lru_index = SIZE_MAX;

        std::array<u32, MAX_MIP_LEVELS> mip_level_offsets{};

        std::vector<ImageViewInfo> image_view_infos;
        std::vector<ImageViewId> image_view_ids;

        boost::container::small_vector<u32, 16> slice_offsets;
        boost::container::small_vector<SubresourceBase, 16> slice_subresources;

        std::vector<AliasedImage> aliased_images;
        std::vector<ImageId> overlapping_images;
        ImageMapId map_view_id{};
};

struct ImageMapView {
        explicit ImageMapView(size_t size, ImageId image_id);
        size_t size;
        ImageId image_id;
        bool picked{};
};

struct ImageAllocBase {
        std::vector<ImageId> images;
};

auto AddImageAlias(ImageBase& lhs, ImageBase& rhs, ImageId lhs_id, ImageId rhs_id) -> bool;

}  // namespace render::texture