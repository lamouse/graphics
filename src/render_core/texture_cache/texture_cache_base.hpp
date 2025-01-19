#pragma once

#include "common/scratch_buffer.h"
#include "common/slot_vector.hpp"
#include "common/thread_worker.hpp"
#include "common/literals.hpp"
#include "render_core/surface.hpp"

#include <boost/container/small_vector.hpp>
#include "render_core/texture/types.hpp"

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

    private:
        Runtime& runtime;
};

}  // namespace render::texture