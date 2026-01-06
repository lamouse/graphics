export module render.buffer.cache_base;
import common.types;

export namespace render::buffer {

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
        [[nodiscard]] auto sizeBytes() const -> size_t { return size_bytes; }

    private:
        size_t size_bytes = 0;
};

}  // namespace render::buffer
