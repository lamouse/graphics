#pragma once

#include "render_core/buffer_cache/buffer_cache_base.hpp"
#include "render_core/texture/types.hpp"
#include "render_core/fixed_pipeline_state.h"
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
auto BufferCache<P>::BindIndexBuffer(void* data, u32 size) -> BufferId {
    if(index_buffer.size != size){
        index_buffer.buffer_id = CreateBuffer(size);
        index_buffer.size = size;
    }

    auto& buffer = slot_buffers[index_buffer.buffer_id];
    if constexpr (USE_MEMORY_MAPS_FOR_UPLOADS) {
        auto upload_staging = runtime.UploadStagingBuffer(size);
        std::array<texture::BufferCopy, 1> copies{{texture::BufferCopy{
            .src_offset = upload_staging.offset, .dst_offset = 0, .size = size}}};
        std::memcpy(upload_staging.mapped_span.data(), data, size);
        runtime.CopyBuffer(buffer, upload_staging.buffer, copies, true);
    }
    // TODO buffer.MarkUsage(offset, size);
    runtime.BindIndexBuffer(PrimitiveTopology::Triangles, IndexFormat::UnsignedShort, 0, 1, buffer,
                            0, size);
    indirect_buffer_binding.buffer_id = index_buffer.buffer_id;
    indirect_buffer_binding.size = size;
    return index_buffer.buffer_id;
}

template <class P>
void BufferCache<P>::TouchBuffer(Buffer& buffer, BufferId buffer_id) noexcept {
    if (buffer_id != NULL_BUFFER_ID) {
        lru_cache.Touch(buffer.getLRUID(), frame_tick);
    }
}

template <class P>
auto BufferCache<P>::BindVertexBuffers(void* data, u32 size) -> BufferId {
    HostBindings<typename P::Buffer> host_bindings;
    // 这里需要后续处理
    bool any_valid{false};
    for (u32 index = 0; index < 1; ++index) {
        const Binding& binding = vertex_buffers[index];
        if (binding.buffer_id == NULL_BUFFER_ID || binding.size != size) {
            auto buffer_id = CreateBuffer(size);
            auto& buffer = slot_buffers[buffer_id];
            auto upload_staging = runtime.UploadStagingBuffer(size);
            std::array<texture::BufferCopy, 1> copies{{texture::BufferCopy{
                .src_offset = upload_staging.offset, .dst_offset = 0, .size = size}}};
            std::memcpy(upload_staging.mapped_span.data(), data, size);
            runtime.CopyBuffer(buffer, upload_staging.buffer, copies, true);
            vertex_buffers[index] = Binding{.size = size, .buffer_id = buffer_id};
        }
//        Buffer& buffer = slot_buffers[binding.buffer_id];
        // TouchBuffer(buffer, binding.buffer_id);

        host_bindings.min_index = std::min(host_bindings.min_index, index);
        host_bindings.max_index = std::max(host_bindings.max_index, static_cast<u32>(1)); //待修复
        any_valid = true;
    }

    for (u32 index = host_bindings.min_index; index < host_bindings.max_index; index++) {
        auto& buffer = slot_buffers[vertex_buffers[index].buffer_id];
        host_bindings.buffers.push_back(&buffer);
        host_bindings.offsets.push_back(0);
        host_bindings.sizes.push_back(size);
        host_bindings.strides.push_back(32);
        runtime.BindVertexBuffers(host_bindings);
    }
    return {};
}

template <class P>
auto BufferCache<P>::BindUniforBuffers(size_t stage, u32 index, void* data, u32 size) -> BufferId {
    Binding bind = uniform_buffers[stage][index];
    if (!bind.buffer_id || bind.size != size) {
        bind.buffer_id = CreateBuffer(size);
        bind.size = size;
    }
    auto& buffer = slot_buffers[bind.buffer_id];
    auto upload_staging = runtime.UploadStagingBuffer(size);
    std::array<texture::BufferCopy, 1> copies{
        {texture::BufferCopy{.src_offset = upload_staging.offset, .dst_offset = 0, .size = size}}};
    std::memcpy(upload_staging.mapped_span.data(), data, size);
    runtime.CopyBuffer(buffer, upload_staging.buffer, copies, true);

    uniform_buffers[stage][index] = bind;
    runtime.BindUniformBuffer(buffer, 0, size);
    return bind.buffer_id;
}

template <class P>
auto BufferCache<P>::GetDrawIndirectCount() -> std::pair<typename BufferCache<P>::Buffer*, u32> {
    auto& buffer = slot_buffers[count_buffer_binding.buffer_id];
    return std::make_pair(&buffer, 0);
}

template <class P>
auto BufferCache<P>::GetDrawIndirectBuffer() -> std::pair<typename BufferCache<P>::Buffer*, u32> {
    auto& buffer = slot_buffers[indirect_buffer_binding.buffer_id];
    return std::make_pair(&buffer, 0);
}

}  // namespace render::buffer