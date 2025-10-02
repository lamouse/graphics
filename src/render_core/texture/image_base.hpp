#pragma once

#include <array>
#include <vector>
#include <boost/container/small_vector.hpp>

#include "common/common_types.hpp"
#include "render_core/texture/image_info.hpp"
#include "render_core/texture/image_view_info.hpp"
#include "render_core/texture/types.hpp"

namespace render::texture {

struct ImageViewInfo;
struct NullImageParams {};

struct ImageBase {
        explicit ImageBase(const ImageInfo& info);
        explicit ImageBase(const NullImageParams&);

        [[nodiscard]] auto FindView(const ImageViewInfo& view_info) const noexcept -> ImageViewId;

        void InsertView(const ImageViewInfo& view_info, ImageViewId image_view_id);

        [[nodiscard]] auto IsSafeDownload() const noexcept -> bool;


        [[nodiscard]] auto HasScaled() const -> bool { return has_scaled; }

        ImageInfo info;

        u32 guest_size_bytes = 0;
        u32 converted_size_bytes = 0;
        u32 scale_rating = 0;
        u64 scale_tick = 0;
        bool has_scaled = false;

        size_t channel = 0;

        u64 modification_tick = 0;
        size_t lru_index = SIZE_MAX;

        std::array<u32, MAX_MIP_LEVELS> mip_level_offsets{};

        std::vector<ImageViewInfo> image_view_infos;
        std::vector<ImageViewId> image_view_ids;

        boost::container::small_vector<u32, 16> slice_offsets;
        boost::container::small_vector<SubresourceBase, 16> slice_subresources;

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

}  // namespace render::texture