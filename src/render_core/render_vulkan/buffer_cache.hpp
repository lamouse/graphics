#pragma once
#include <vulkan/vulkan.hpp>
#include "buffer_cache/buffer_base.hpp"
#include "surface.hpp"
#include "buffer_cache/usage_tracker.hpp"
namespace render_vulkan {
namespace resource {
class DescriptorPool;
}
class Device;
namespace scheduler {
class Scheduler;
}
namespace buffer {

class BufferCacheRuntime;
class Buffer : public render::buffer::BufferBase {
    public:
        explicit Buffer(BufferCacheRuntime&, render::buffer::NullBufferParams null_params);
        explicit Buffer(BufferCacheRuntime& runtime, VAddr cpu_addr_, u64 size_bytes_);

        [[nodiscard]] auto view(u32 offset, u32 size, render::surface::PixelFormat format)
            -> vk::BufferView;

        [[nodiscard]] auto handle() const noexcept -> vk::Buffer { return buffer; }

        [[nodiscard]] auto IsRegionUsed(u64 offset, u64 size) const noexcept -> bool {
            return tracker.IsUsed(offset, size);
        }

        void MarkUsage(u64 offset, u64 size) noexcept { tracker.Track(offset, size); }

        void ResetUsageTracking() noexcept { tracker.Reset(); }

        explicit operator vk::Buffer() const noexcept { return buffer; }

    private:
        struct BufferView {
                u32 offset;
                u32 size;
                render::surface::PixelFormat format;
                vk::BufferView handle;
        };

        const Device* device{};
        vk::Buffer buffer;
        std::vector<BufferView> views;
        render::buffer::UsageTracker tracker;
        bool is_null{};
};

class QuadArrayIndexBuffer;
class QuadStripIndexBuffer;

}  // namespace buffer
}  // namespace render_vulkan