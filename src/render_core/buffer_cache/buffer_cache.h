#pragma once

#include "render_core/buffer_cache/buffer_cache_base.hpp"
#include "render_core/texture/types.hpp"
#include <algorithm>
#undef min
#undef max
namespace render::buffer {
template <class P>
BufferCache<P>::BufferCache(Runtime& runtime_) : runtime{runtime_} {}
template <class P>
auto BufferCache<P>::CreateBuffer(u32 wanted_size) -> BufferId {
    const BufferId new_buffer_id = slot_buffers.insert(runtime, wanted_size);
    auto& new_buffer = slot_buffers[new_buffer_id];
    const size_t size_bytes = wanted_size;
    runtime.ClearBuffer(new_buffer, 0, size_bytes, 0);
    new_buffer.MarkUsage(0, size_bytes);
    return new_buffer_id;
}
template <class P>
auto BufferCache<P>::addIndexBuffer(const void* data, u32 size) -> BufferId {
    auto buffer_id = CreateBuffer(size);
    auto& buffer = slot_buffers[buffer_id];
    auto upload_staging = runtime.UploadStagingBuffer(size);
    std::array<texture::BufferCopy, 1> copies{
        {texture::BufferCopy{.src_offset = upload_staging.offset, .dst_offset = 0, .size = size}}};
    std::memcpy(upload_staging.mapped_span.data(), data, size);
    runtime.CopyBuffer(buffer, upload_staging.buffer, copies, true);
    return buffer_id;
}

template <class P>
void BufferCache<P>::BindIndexBuffer(IndexFormat format, BufferId id) {
    runtime.BindIndexBuffer(format, slot_buffers[id]);
}

template <class P>
void BufferCache<P>::BindVertexBuffers(BufferId id, u32 size) {
    HostBindings<typename P::Buffer> host_bindings;
    for (u32 index = 0; index < 1; ++index) {
        // TouchBuffer(buffer, id);
        host_bindings.min_index = std::min(host_bindings.min_index, index);
        host_bindings.max_index = std::max(host_bindings.max_index, static_cast<u32>(1));  // 待修复
    }
    for (u32 index = host_bindings.min_index; index < host_bindings.max_index; index++) {
        auto& buffer = slot_buffers[id];
        host_bindings.buffers.push_back(&buffer);
        host_bindings.offsets.push_back(0);
        host_bindings.sizes.push_back(size);
        host_bindings.strides.push_back(32);
        runtime.BindVertexBuffers(host_bindings);
    }
}

template <class P>
void BufferCache<P>::TouchBuffer(Buffer& buffer, BufferId buffer_id) noexcept {
    if (buffer_id != NULL_BUFFER_ID) {
        lru_cache.Touch(buffer.getLRUID(), frame_tick);
    }
}

template <class P>
auto BufferCache<P>::addVertexBuffer(const void* data, u32 size) -> BufferId {
    auto buffer_id = CreateBuffer(size);
    auto& buffer = slot_buffers[buffer_id];
    auto upload_staging = runtime.UploadStagingBuffer(size);
    std::array<texture::BufferCopy, 1> copies{
        {texture::BufferCopy{.src_offset = upload_staging.offset, .dst_offset = 0, .size = size}}};
    std::memcpy(upload_staging.mapped_span.data(), data, size);
    runtime.CopyBuffer(buffer, upload_staging.buffer, copies, true);
    return buffer_id;
}

template <class P>
void BufferCache<P>::BindStageBuffers(size_t stage) {
    // BindHostGraphicsStorageBuffers(stage);
    // BindHostGraphicsTextureBuffers(stage);
}

template <class P>
void BufferCache<P>::TickFrame() {
    runtime.TickFrame(slot_buffers);

    // Calculate hits and shots and move hit bits to the right
    const u32 hits = std::reduce(uniform_cache_hits.begin(), uniform_cache_hits.end());
    const u32 shots = std::reduce(uniform_cache_shots.begin(), uniform_cache_shots.end());
    std::copy_n(uniform_cache_hits.begin(), uniform_cache_hits.size() - 1,
                uniform_cache_hits.begin() + 1);
    std::copy_n(uniform_cache_shots.begin(), uniform_cache_shots.size() - 1,
                uniform_cache_shots.begin() + 1);
    uniform_cache_hits[0] = 0;
    uniform_cache_shots[0] = 0;

    const bool skip_preferred = hits * 256 < shots * 251;
    uniform_buffer_skip_cache_size = skip_preferred ? DEFAULT_SKIP_CACHE_SIZE : 0;

    // If we can obtain the memory info, use it instead of the estimate.
    if (runtime.CanReportMemoryUsage()) {
        total_used_memory = runtime.GetDeviceMemoryUsage();
    }
    // if (total_used_memory >= minimum_memory) {
    //     RunGarbageCollector();
    // }
    ++frame_tick;
    delayed_destruction_ring.Tick();

    for (auto& buffer : async_buffers_death_ring) {
        runtime.FreeDeferredStagingBuffer(buffer);
    }
    async_buffers_death_ring.clear();
}

template <class P>
void BufferCache<P>::BindUniformBuffer(std::span<const std::byte> data) {
    auto map = runtime.BindMappedUniformBuffer(0, 0, data.size());
    std::memcpy(map.data(), data.data(), data.size());
}

template <class P>
void BufferCache<P>::DoUpdateComputeBuffers() {
    UpdateComputeUniformBuffers();
    UpdateComputeStorageBuffers();
    UpdateComputeTextureBuffers();
}

template <class P>
void BufferCache<P>::UpdateComputeUniformBuffers() {

}

template <class P>
void BufferCache<P>::UpdateComputeStorageBuffers() {

}

template <class P>
void BufferCache<P>::UpdateComputeTextureBuffers() {

}


}  // namespace render::buffer