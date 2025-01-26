#pragma once

#include "common/scratch_buffer.h"
#include "common/slot_vector.hpp"
#include "common/thread_worker.hpp"
#include "common/literals.hpp"
#include "render_core/surface.hpp"

#include <boost/container/small_vector.hpp>
#include "render_core/texture/types.hpp"
#include "render_core/texture/image_info.hpp"
#include "render_core/texture/image_view_info.hpp"
#include "render_core/texture/render_targets.h"
#include <unordered_set>

namespace render::texture {
using namespace common::literals;
struct ImageViewInOut {
        u32 index{};
        bool blacklist{};
        ImageViewId id{};
};

struct AsyncDecodeContext {
        ImageId image_id;
        common::ScratchBuffer<u8> decoded_data;
        boost::container::small_vector<BufferImageCopy, 16> copies;
        std::mutex mutex;
        std::atomic_bool complete;
};

class TextureCacheInfo {
        std::vector<SamplerId> graphics_sampler_ids;
        std::vector<ImageViewId> graphics_image_view_ids;

        std::vector<SamplerId> compute_sampler_ids;
        std::vector<ImageViewId> compute_image_view_ids;
};

template <class P>
class TextureCache : public TextureCacheInfo {
        /// Address shift for caching images into a hash table
        static constexpr u64 ENGINE_PAGEBITS = 20;

        /// Enables debugging features to the texture cache
        static constexpr bool ENABLE_VALIDATION = P::ENABLE_VALIDATION;
        /// Implement blits as copies between framebuffers
        static constexpr bool FRAMEBUFFER_BLITS = P::FRAMEBUFFER_BLITS;
        /// True when some copies have to be emulated
        static constexpr bool HAS_EMULATED_COPIES = P::HAS_EMULATED_COPIES;
        /// True when the API can provide info about the memory of the device.
        static constexpr bool HAS_DEVICE_MEMORY_INFO = P::HAS_DEVICE_MEMORY_INFO;
        /// True when the API can do asynchronous texture downloads.
        static constexpr bool IMPLEMENTS_ASYNC_DOWNLOADS = P::IMPLEMENTS_ASYNC_DOWNLOADS;

        static constexpr size_t UNSET_CHANNEL{std::numeric_limits<size_t>::max()};

        static constexpr s64 TARGET_THRESHOLD = 4_GiB;
        static constexpr s64 DEFAULT_EXPECTED_MEMORY = 1_GiB + 125_MiB;
        static constexpr s64 DEFAULT_CRITICAL_MEMORY = 1_GiB + 625_MiB;
        static constexpr size_t GC_EMERGENCY_COUNTS = 2;

        using Runtime = typename P::Runtime;
        using Image = typename P::Image;
        using ImageAlloc = typename P::ImageAlloc;
        using ImageView = typename P::ImageView;
        using Sampler = typename P::Sampler;
        using Framebuffer = typename P::Framebuffer;
        using AsyncBuffer = typename P::AsyncBuffer;
        using BufferType = typename P::BufferType;

        struct BlitImages {
                ImageId dst_id;
                ImageId src_id;
                surface::PixelFormat dst_format;
                surface::PixelFormat src_format;
        };

    public:
        explicit TextureCache(Runtime&);
        ~TextureCache() = default;

        /// Mark images in a range as modified from the CPU
        void WriteMemory(void* data, size_t size);
        /// Create an image from the given parameters
        [[nodiscard]] auto InsertImage(const ImageInfo& info, RelaxedOptions options) -> ImageId;

        /// Create a new image and join perfectly matching existing images
        /// Remove joined images from the cache
        [[nodiscard]] auto JoinImages(const ImageInfo& info) -> ImageId;

        /// Execute copies from one image to the other, even if they are incompatible
        void CopyImage(ImageId dst_id, ImageId src_id, std::vector<ImageCopy> copies);

        /// Find or create an image view in the given image with the passed parameters
        [[nodiscard]] auto FindOrEmplaceImageView(ImageId image_id, const ImageViewInfo& info)
            -> ImageViewId;

        /// Create a render target from a given image and image view parameters
        [[nodiscard]] auto RenderTargetFromImage(ImageId, const ImageViewInfo& view_info)
            -> std::pair<FramebufferId, ImageViewId>;

        /// Find a framebuffer with the currently bound render targets
        /// UpdateRenderTargets should be called before this
        auto GetFramebuffer() -> Framebuffer*;

        /// Find or create a framebuffer with the given render target parameters
        auto GetFramebufferId(const RenderTargets& key) -> FramebufferId;

    private:
        Runtime& runtime;

        common::SlotVector<Image> slot_images;
        common::SlotVector<ImageView> slot_image_views;
        common::SlotVector<ImageAlloc> slot_image_allocs;
        common::SlotVector<Sampler> slot_samplers;
        common::SlotVector<Framebuffer> slot_framebuffers;

        std::unordered_map<GPUVAddr, ImageAllocId> image_allocs_table;

        common::ThreadWorker texture_decode_worker{1, "TextureDecoder"};
        std::vector<std::unique_ptr<AsyncDecodeContext>> async_decodes;

        // Join caching
        boost::container::small_vector<ImageId, 4> join_overlap_ids;
        std::unordered_set<ImageId> join_overlaps_found;
        boost::container::small_vector<ImageId, 4> join_left_aliased_ids;
        boost::container::small_vector<ImageId, 4> join_right_aliased_ids;
        std::unordered_set<ImageId> join_ignore_textures;
        boost::container::small_vector<ImageId, 4> join_bad_overlap_ids;
        struct JoinCopy {
                bool is_alias;
                ImageId id;
        };
        boost::container::small_vector<JoinCopy, 4> join_copies_to_do;
        std::unordered_map<ImageId, size_t> join_alias_indices;

        RenderTargets render_targets;

        std::unordered_map<RenderTargets, FramebufferId> framebuffers;
};

}  // namespace render::texture