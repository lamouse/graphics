#pragma once
#include "common/common_types.hpp"
namespace render::buffer {

/// Tag for creating null buffers with no storage or size
struct NullBufferParams {};

class BufferBase {
    public:
        explicit BufferBase(u64 size_bytes_) : size_bytes{size_bytes_} {}

        explicit BufferBase(NullBufferParams /*unused*/) {}

        auto operator=(const BufferBase&) -> BufferBase& = delete;
        BufferBase(const BufferBase&) = delete;

        auto operator=(BufferBase&&) -> BufferBase& = default;
        BufferBase(BufferBase&&) = default;
        virtual ~BufferBase() = default;


        /// Increases the likeliness of this being a stream buffer
        void increaseStreamScore(int score) noexcept { stream_score += score; }

        /// Returns the likeliness of this being a stream buffer
        [[nodiscard]] auto streamScore() const noexcept -> int { return stream_score; }


        [[nodiscard]] auto getLRUID() const noexcept -> size_t { return lru_id; }

        void setLRUID(size_t lru_id_) { lru_id = lru_id_; }

        [[nodiscard]] auto sizeBytes() const -> size_t { return size_bytes; }

    private:
        int stream_score = 0;
        size_t lru_id = SIZE_MAX;
        size_t size_bytes = 0;
};

}  // namespace render::buffer