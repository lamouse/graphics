#pragma once
#include "common/common_funcs.hpp"
#include "common/common_types.hpp"
#include <bit>
namespace render::buffer {
enum class BufferFlagBits {
    Picked = 1 << 0,
    CachedWrites = 1 << 1,
    PreemtiveDownload = 1 << 2,
};
DECLARE_ENUM_FLAG_OPERATORS(BufferFlagBits)
/// Tag for creating null buffers with no storage or size
struct NullBufferParams {};

/**
 * Range tracking buffer container.
 *
 * It keeps track of the modified CPU and GPU ranges on a CPU page granularity, notifying the given
 * rasterizer about state changes in the tracking behavior of the buffer.
 *
 * The buffer size and address is forcefully aligned to CPU page boundaries.
 */
class BufferBase {
    public:
        static constexpr u64 BASE_PAGE_BITS = 16;
        static constexpr u64 BASE_PAGE_SIZE = 1ULL << BASE_PAGE_BITS;

        explicit BufferBase(u64 size_bytes_) : size_bytes{size_bytes_} {}

        explicit BufferBase(NullBufferParams /*unused*/) {}

        auto operator=(const BufferBase&) -> BufferBase& = delete;
        BufferBase(const BufferBase&) = delete;

        auto operator=(BufferBase&&) -> BufferBase& = default;
        BufferBase(BufferBase&&) = default;
        virtual ~BufferBase() = default;
        /// Mark buffer as picked
        void pick() noexcept { flags |= BufferFlagBits::Picked; }

        void markPreemtiveDownload() noexcept { flags |= BufferFlagBits::PreemtiveDownload; }

        /// Unmark buffer as picked
        void unpick() noexcept { flags &= ~BufferFlagBits::Picked; }

        /// Increases the likeliness of this being a stream buffer
        void increaseStreamScore(int score) noexcept { stream_score += score; }

        /// Returns the likeliness of this being a stream buffer
        [[nodiscard]] auto streamScore() const noexcept -> int { return stream_score; }

        /// Returns true if the buffer has been marked as picked
        [[nodiscard]] auto isPicked() const noexcept -> bool {
            return True(flags & BufferFlagBits::Picked);
        }

        /// Returns true when the buffer has pending cached writes
        [[nodiscard]] auto hasCachedWrites() const noexcept -> bool {
            return True(flags & BufferFlagBits::CachedWrites);
        }

        [[nodiscard]] auto IsPreemtiveDownload() const noexcept -> bool {
            return True(flags & BufferFlagBits::PreemtiveDownload);
        }

        [[nodiscard]] auto getLRUID() const noexcept -> size_t { return lru_id; }

        void setLRUID(size_t lru_id_) { lru_id = lru_id_; }

        [[nodiscard]] auto sizeBytes() const -> size_t { return size_bytes; }

    private:
        BufferFlagBits flags{};
        int stream_score = 0;
        size_t lru_id = SIZE_MAX;
        size_t size_bytes = 0;
};

}  // namespace render::buffer