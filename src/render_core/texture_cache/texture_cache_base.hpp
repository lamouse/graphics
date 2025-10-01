#pragma once

#include "common/scratch_buffer.h"
#include "common/slot_vector.hpp"
#include "common/thread_worker.hpp"
#include "common/literals.hpp"

#include <boost/container/small_vector.hpp>
#include "render_core/texture/types.hpp"
#include "render_core/texture/image_info.hpp"
#include "render_core/texture/image_view_info.hpp"
#include "render_core/texture/render_targets.h"
#include "render_core/framebufferConfig.hpp"
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
    public:
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

    public:
        explicit TextureCache(Runtime&);
        ~TextureCache() = default;
        /// Notify the cache that a new frame has been queued
        void TickFrame();
        /// Create an image from the given parameters
        [[nodiscard]] auto InsertImage(const ImageInfo& info) -> ImageId;

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

        /// Find or create a sampler from a guest descriptor sampler
        [[nodiscard]] auto FindSampler(u32 index) -> SamplerId;

        /// Find a framebuffer with the currently bound render targets
        /// UpdateRenderTargets should be called before this
        auto GetFramebuffer(const RenderTargets& key) -> Framebuffer*;

        /// Returns the currently bound framebuffer, or nullptr if none is bound
        auto GetFramebuffer() -> Framebuffer*;
        /// Find or create a framebuffer with the given render target parameters
        auto GetFramebufferId(const RenderTargets& key) -> FramebufferId;

        /// Get the sampler from the graphics descriptor table in the specified index
        auto GetGraphicsSampler(u32 index) -> Sampler*;

        /// Get the sampler from the compute descriptor table in the specified index
        auto GetComputeSampler(u32 index) -> Sampler*;

        /// Get the sampler id from the graphics descriptor table in the specified index
        auto GetGraphicsSamplerId(u32 index) -> SamplerId;
        // 添加一个图像
        auto addGraphics(const ImageInfo& info) -> std::pair<ImageViewId, SamplerId>;

        //添加一个纹理
        auto addTexture(const Extent2D& extent, std::span<unsigned char> data) -> std::pair<ImageViewId, SamplerId>;

        /// Get the sampler id from the compute descriptor table in the specified index
        auto GetComputeSamplerId(u32 index) -> SamplerId;

        /// Return a constant reference to the given sampler id
        [[nodiscard]] auto GetSampler(SamplerId id) const noexcept -> const Sampler&;

        /// Return a reference to the given sampler id
        [[nodiscard]] auto GetSampler(SamplerId id) noexcept -> Sampler&;

        /// Return a reference to the given sampler id
        [[nodiscard]] auto CreateSampler(ImageViewId id) -> SamplerId;
        /// Find or create an image view from a guest descriptor
        [[nodiscard]] auto FindImageView(const ImageInfo& info) -> ImageViewId;

        /// Create a new image view from a guest descriptor
        [[nodiscard]] auto CreateImageView(const ImageInfo& info) -> ImageViewId;
        /// Find or create an image from the given parameters
        [[nodiscard]] auto FindOrInsertImage(const ImageInfo& info) -> ImageId;

        /// Find an image from the given parameters
        [[nodiscard]] auto FindImage(const ImageInfo& info) -> ImageId;
        /// Return a constant reference to the given image view id
        [[nodiscard]] auto GetImageView(ImageViewId id) const noexcept -> const ImageView&;

        /// Return a reference to the given image view id
        [[nodiscard]] auto GetImageView(ImageViewId id) noexcept -> ImageView&;
        /// Get the imageview from the graphics descriptor table in the specified index
        [[nodiscard]] auto GetImageView(u32 index) noexcept -> ImageView&;
        /// Refresh the state for graphics image view and sampler descriptors
        void SynchronizeGraphicsDescriptors();
        std::recursive_mutex mutex;

        /// Try to find a cached image view in the given CPU address
        [[nodiscard]] auto TryFindFramebufferImageView(const frame::FramebufferConfig& config)
            -> std::pair<ImageView*, bool>;

        /// Update bound render targets
        auto UpdateRenderTargets(std::span<ImageInfo> infos, Extent2D extent) -> RenderTargets;
        /**
         * 这里主要针对添加的图片
         */
        void setCurrentImage(ImageViewId view_id, SamplerId sampler_id);
        /**
         *
         * @return 这里主要针对上次设置的的图片
         */
        auto getCurrentImage() -> std::pair<ImageView*, Sampler*>;

    private:
        Runtime& runtime;
        std::vector<FramebufferId> frame_buffer_ids;
        int current_framebuffer_index{-1};
        common::SlotVector<Image> slot_images;
        common::SlotVector<ImageView> slot_image_views;
        common::SlotVector<ImageAlloc> slot_image_allocs;
        common::SlotVector<Sampler> slot_samplers;
        common::SlotVector<Framebuffer> slot_framebuffers;
        std::vector<std::pair<FramebufferId, ImageViewId>> framebuffer_views;
        common::ThreadWorker texture_decode_worker{1, "TextureDecoder"};
        std::vector<std::unique_ptr<AsyncDecodeContext>> async_decodes;
        std::deque<AsyncBuffer> async_buffers_death_ring;

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
        std::pair<ImageViewId, SamplerId> current_image_view;
        std::unordered_map<RenderTargets, FramebufferId> framebuffers;
        u64 frame_tick = 0;
};

}  // namespace render::texture