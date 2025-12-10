#pragma once

#include "render_core/buffer_cache/buffer_cache_base.hpp"
#include "render_core/texture/types.hpp"
#include <algorithm>
#include <utility>
#include <memory>
namespace render::buffer {
template <class P>
BufferCache<P>::BufferCache(Runtime& runtime_) : runtime{runtime_} {}
template <class P>
auto BufferCache<P>::CreateBuffer(u32 wanted_size) -> BufferId {
    const BufferId new_buffer_id = slot_buffers.insert(runtime, wanted_size);
    auto& new_buffer = slot_buffers[new_buffer_id];
    const size_t size_bytes = wanted_size;
    runtime.ClearBuffer(new_buffer, 0, size_bytes, 0);
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
void BufferCache<P>::BindVertexBuffers(BufferId id, u32 size, u64 stride) {
    auto host_bindings = std::make_unique<HostBindings<typename P::Buffer>>();
    bool any_valid{false};
    for (u32 index = 0; index < 1; ++index) {
        // TouchBuffer(buffer, id);
        host_bindings->min_index = std::min(host_bindings->min_index, index);
        host_bindings->max_index = std::max(host_bindings->max_index, index);  // 待修复
        any_valid = true;
    }
    if (any_valid) {
        host_bindings->max_index++;
        for (u32 index = host_bindings->min_index; index < host_bindings->max_index; index++) {
            auto& buffer = slot_buffers[id];
            host_bindings->buffers.push_back(&buffer);
            host_bindings->offsets.push_back(0);
            host_bindings->sizes.push_back(size);
            host_bindings->strides.push_back(stride);
        }
        runtime.BindVertexBuffers(std::move(host_bindings));
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
void BufferCache<P>::TickFrame() {
    runtime.TickFrame();
    // If we can obtain the memory info, use it instead of the estimate.
    if (runtime.CanReportMemoryUsage()) {
        total_used_memory = runtime.GetDeviceMemoryUsage();
    }
}

template <class P>
void BufferCache<P>::BindGraphicUniformBuffer() {
    if (graphic_uniform_buffers.empty()) {
        return;
    }
    for (const auto& ubo : graphic_uniform_buffers) {
        auto map = runtime.BindMappedUniformBuffer(0, 0, ubo.size());
        std::memcpy(map.data(), ubo.data(), ubo.size());
    }

    graphic_uniform_buffers = {};
}
template <class P>
void BufferCache<P>::UploadGraphicUniformBuffer(std::span<std::span<const std::byte>> data) {
    graphic_uniform_buffers = data;
}
template <class P>
void BufferCache<P>::UploadComputeUniformBuffer(std::vector<std::span<const std::byte>> data) {
    compute_uniform_buffers = std::move(data);
}

template <class P>
void BufferCache<P>::DoUpdateComputeBuffers() {
    UpdateComputeUniformBuffers();
    UpdateComputeStorageBuffers();
    UpdateComputeTextureBuffers();
}

template <class P>
void BufferCache<P>::UpdateComputeUniformBuffers() {
    if (compute_uniform_buffers.empty()) {
        return;
    }
    for (const auto& ubo : compute_uniform_buffers) {
        auto map = runtime.BindMappedUniformBuffer(0, 0, ubo.size());
        std::memcpy(map.data(), ubo.data(), ubo.size());
    }
    compute_uniform_buffers = {};
}

template <class P>
void BufferCache<P>::UpdateComputeStorageBuffers() {
    for (std::uint32_t i = 0; i < compute_storage_buffers_size; i++) {
        auto& buffer = slot_buffers[compute_storage_buffers.at(i).buffer_id];
        runtime.BindStorageBuffer(buffer, 0, compute_storage_buffers.at(i).size);
    }
    compute_storage_buffers_size = 0;
}

template <class P>
void BufferCache<P>::UpdateComputeTextureBuffers() {}

template <class P>
void BufferCache<P>::bindComputeStorageBuffers(BufferId id) {
    auto& buffer = slot_buffers[id];
    compute_storage_buffers.at(compute_storage_buffers_size++) =
        Binding{.size = static_cast<u32>(buffer.sizeBytes()), .buffer_id = id};
}
template <class P>
void BufferCache<P>::UploadPushConstants(std::span<const std::byte> data) {
    push_constants = data;
}
template <class P>
auto BufferCache<P>::GetPushConstants() -> std::span<const std::byte> {
    auto tmp = push_constants;
    push_constants = {};
    return tmp;
}

}  // namespace render::buffer