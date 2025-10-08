#pragma once
#include "common/common_types.hpp"
#include "common/scope_exit.h"
#include "common/common_funcs.hpp"
#include "common/slot_vector.hpp"
#include "common/div_ceil.hpp"
#include "common/lru_cache.h"
#include "common/literals.hpp"
#include "render_core/texture/types.hpp"
#include "render_core/surface.hpp"
#include "render_core/delayed_destruction_ring.h"
#include "render_core/fixed_pipeline_state.h"
#include <boost/container/small_vector.hpp>
#include <deque>
#include <mutex>
#include <span>

namespace render::buffer {

using BufferId = common::SlotId;
using namespace common::literals;
using namespace surface;
#ifdef __APPLE__
constexpr u32 NUM_VERTEX_BUFFERS = 16;
#else
constexpr u32 NUM_VERTEX_BUFFERS = 32;
#endif
constexpr u32 NUM_TRANSFORM_FEEDBACK_BUFFERS = 4;
constexpr u32 NUM_GRAPHICS_UNIFORM_BUFFERS = 18;
constexpr u32 NUM_COMPUTE_UNIFORM_BUFFERS = 8;
constexpr u32 NUM_STORAGE_BUFFERS = 16;
constexpr u32 NUM_TEXTURE_BUFFERS = 32;
constexpr u32 NUM_STAGES = 5;

using UniformBufferSizes = std::array<std::array<u32, NUM_GRAPHICS_UNIFORM_BUFFERS>, NUM_STAGES>;
using ComputeUniformBufferSizes = std::array<u32, NUM_COMPUTE_UNIFORM_BUFFERS>;

enum class ObtainBufferSynchronize : u32 {
    NoSynchronize = 0,
    FullSynchronize = 1,
    SynchronizeNoDirty = 2,
};

enum class ObtainBufferOperation : u32 {
    DoNothing = 0,
    MarkAsWritten = 1,
    DiscardWrite = 2,
    MarkQuery = 3,
};

static constexpr BufferId NULL_BUFFER_ID{0};
static constexpr u32 DEFAULT_SKIP_CACHE_SIZE = static_cast<u32>(4_KiB);

struct Binding {
        u32 size{};
        BufferId buffer_id;
};

struct TextureBufferBinding : Binding {
        PixelFormat format;
};

static constexpr Binding NULL_BINDING{
    .size = 0,
    .buffer_id = NULL_BUFFER_ID,
};

template <typename Buffer>
struct HostBindings {
        boost::container::small_vector<Buffer*, NUM_VERTEX_BUFFERS> buffers;
        boost::container::small_vector<u64, NUM_VERTEX_BUFFERS> offsets;
        boost::container::small_vector<u64, NUM_VERTEX_BUFFERS> sizes;
        boost::container::small_vector<u64, NUM_VERTEX_BUFFERS> strides;
        u32 min_index{NUM_VERTEX_BUFFERS};
        u32 max_index{0};
};

struct IndirectParams {
        bool is_byte_count;
        bool is_indexed;
        bool include_count;
        size_t buffer_size;
        size_t max_draw_counts;
        size_t stride;
};

class BufferCacheInfo {
    public:
        BufferCacheInfo() noexcept = default;
        CLASS_NON_COPYABLE(BufferCacheInfo);

        Binding index_buffer;
        std::array<Binding, NUM_VERTEX_BUFFERS> vertex_buffers;
        std::array<std::array<Binding, NUM_GRAPHICS_UNIFORM_BUFFERS>, NUM_STAGES> uniform_buffers;
        std::array<std::array<Binding, NUM_STORAGE_BUFFERS>, NUM_STAGES> storage_buffers;
        std::array<std::array<TextureBufferBinding, NUM_TEXTURE_BUFFERS>, NUM_STAGES>
            texture_buffers;
        std::array<Binding, NUM_TRANSFORM_FEEDBACK_BUFFERS> transform_feedback_buffers;
        Binding count_buffer_binding;
        Binding indirect_buffer_binding;

        std::array<Binding, NUM_COMPUTE_UNIFORM_BUFFERS> compute_uniform_buffers;
        std::array<Binding, NUM_STORAGE_BUFFERS> compute_storage_buffers;
        std::array<TextureBufferBinding, NUM_TEXTURE_BUFFERS> compute_texture_buffers;

        std::array<u32, NUM_STAGES> enabled_uniform_buffer_masks{};
        u32 enabled_compute_uniform_buffer_mask = 0;

        const UniformBufferSizes* uniform_buffer_sizes{};
        const ComputeUniformBufferSizes* compute_uniform_buffer_sizes{};

        std::array<u32, NUM_STAGES> enabled_storage_buffers{};
        std::array<u32, NUM_STAGES> written_storage_buffers{};
        u32 enabled_compute_storage_buffers = 0;
        u32 written_compute_storage_buffers = 0;

        std::array<u32, NUM_STAGES> enabled_texture_buffers{};
        std::array<u32, NUM_STAGES> written_texture_buffers{};
        std::array<u32, NUM_STAGES> image_texture_buffers{};
        u32 enabled_compute_texture_buffers = 0;
        u32 written_compute_texture_buffers = 0;
        u32 image_compute_texture_buffers = 0;

        std::array<u32, 16> uniform_cache_hits{};
        std::array<u32, 16> uniform_cache_shots{};

        u32 uniform_buffer_skip_cache_size = DEFAULT_SKIP_CACHE_SIZE;

        bool has_deleted_buffers = false;

        std::array<u32, NUM_STAGES> dirty_uniform_buffers{};
        std::array<u32, NUM_STAGES> fast_bound_uniform_buffers{};
        std::array<std::array<u32, NUM_GRAPHICS_UNIFORM_BUFFERS>, NUM_STAGES>
            uniform_buffer_binding_sizes{};
};

template <class P>
class BufferCache : public BufferCacheInfo {
        static constexpr s64 DEFAULT_EXPECTED_MEMORY = 512_MiB;
        static constexpr s64 DEFAULT_CRITICAL_MEMORY = 1_GiB;
        static constexpr s64 TARGET_THRESHOLD = 4_GiB;

        // Debug Flags.

        static constexpr bool DISABLE_DOWNLOADS = true;

        using Runtime = typename P::Runtime;
        using Buffer = typename P::Buffer;
        using Async_Buffer = typename P::Async_Buffer;

        struct OverlapResult {
                boost::container::small_vector<BufferId, 16> ids;
                bool has_stream_leap = false;
        };

    public:
        explicit BufferCache(Runtime& runtime_);
        void TickFrame();
        auto addVertexBuffer(const void* data, u32 size) -> BufferId;
        auto addIndexBuffer(const void* data, u32 size) -> BufferId;
        auto addUniformBuffer(u32 size) -> BufferId;
        void BindIndexBuffer(IndexFormat format, BufferId id);
        void BindVertexBuffers(BufferId id, u32 size);
        void BindUniformBuffer(std::span<const std::byte> data);
        void BindUniformBuffers(BufferId id, void* data, size_t size);
        auto BindUniformBuffers(size_t stage, u32 index, void* data, u32 size) -> BufferId;
        void BindStageBuffers(size_t stage);
        void BindCurrentUniformBuffers();
        void setCurrentUniformBuffer(BufferId id, u32 size);
        [[nodiscard]] auto GetDrawIndirectCount() -> std::pair<Buffer*, u32>;

        [[nodiscard]] auto GetDrawIndirectBuffer() -> std::pair<Buffer*, u32>;

        [[nodiscard]] auto CreateBuffer(u32 wanted_size) -> BufferId;
        ~BufferCache() = default;
        std::recursive_mutex mutex;

    private:
        template <typename Func>
        static void ForEachEnabledBit(u32 enabled_mask, Func&& func) {
            for (u32 index = 0; enabled_mask != 0; ++index, enabled_mask >>= 1) {
                const int disabled_bits = std::countr_zero(enabled_mask);
                index += disabled_bits;
                enabled_mask >>= disabled_bits;
                func(index);
            }
        }

        void TouchBuffer(Buffer& buffer, BufferId buffer_id) noexcept;

        void BindGraphicsUniformBuffers(size_t stage);
        void BindGraphicsUniformBuffer(size_t stage, u32 index, bool needs_bind);

        Runtime& runtime;
        common::SlotVector<Buffer> slot_buffers;
        DelayedDestructionRing<Buffer, 8> delayed_destruction_ring;

        IndirectParams* current_draw_indirect{};
        Binding current_uniform_buffer{};
        u32 last_index_count = 0;

        // Async Buffers
        std::deque<boost::container::small_vector<texture::BufferCopy, 4>> pending_downloads;

        std::deque<Async_Buffer> async_buffers_death_ring;

        size_t immediate_buffer_capacity = 0;

        struct LRUItemParams {
                using ObjectType = BufferId;
                using TickType = u64;
        };
        common::LeastRecentlyUsedCache<LRUItemParams> lru_cache;
        u64 frame_tick = 0;
        u64 total_used_memory = 0;
        u64 minimum_memory = 0;
        u64 critical_memory = 0;
        BufferId inline_buffer_id;
};

}  // namespace render::buffer