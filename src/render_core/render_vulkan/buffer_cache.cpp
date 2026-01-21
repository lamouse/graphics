module;
#include "common/alignment.h"

#include <boost/container/small_vector.hpp>
#include <vulkan/vulkan.hpp>
#include <spdlog/spdlog.h>

#if defined(MemoryBarrier)
#undef MemoryBarrier
#undef min
#undef max
#endif

module render.vulkan.buffer_cache;
import render.vulkan.common;
import render.vulkan.scheduler;
import render.texture.types;
import render.buffer.cache;
import render.buffer.cache_base;
import render.surface.format;
namespace render::vulkan {

namespace {
auto MakeBufferCopy(const texture::BufferCopy& copy) -> vk::BufferCopy {
    return vk::BufferCopy{
        copy.src_offset,
        copy.dst_offset,
        copy.size,
    };
}

auto ToIndexFormat(IndexFormat index_format) -> vk::IndexType {
    switch (index_format) {
        case IndexFormat::UnsignedByte:
            return vk::IndexType::eUint8EXT;
        case IndexFormat::UnsignedShort:
            return vk::IndexType::eUint16;
        case IndexFormat::UnsignedInt:
            return vk::IndexType::eUint32;
    }
    spdlog::warn("Unimplemented index_format={}", static_cast<int>(index_format));
    return {};
}

auto CreateBuffer(const Device& device, const MemoryAllocator& memory_allocator, u64 size)
    -> Buffer {
    VkBufferUsageFlags flags =
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
        VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT |
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
        VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
    if (device.IsExtTransformFeedbackSupported()) {
        flags |= VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_BUFFER_BIT_EXT;
    }
    if (device.IsExtConditionalRendering()) {
        flags |= VK_BUFFER_USAGE_CONDITIONAL_RENDERING_BIT_EXT;
    }
    const VkBufferCreateInfo buffer_ci = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = size,
        .usage = flags,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
    };
    return memory_allocator.createBuffer(buffer_ci, MemoryUsage::DeviceLocal);
}

}  // namespace

BaseBufferCache::BaseBufferCache(BufferCacheRuntime& runtime, buffer::NullBufferParams null_params)
    : buffer::BufferBase(null_params) {
    if (runtime.device.HasNullDescriptor()) {
        return;
    }
    device = &runtime.device;
    buffer = runtime.CreateNullBuffer();
    is_null = true;
}

BaseBufferCache::BaseBufferCache(BufferCacheRuntime& runtime, u64 size_bytes_)
    : buffer::BufferBase(size_bytes_),
      device{&runtime.device},
      buffer{CreateBuffer(*device, runtime.memory_allocator, sizeBytes())} {}

auto BaseBufferCache::View(u32 offset, u32 size, surface::PixelFormat format) -> vk::BufferView {
    if (!device) {
        // Null buffer supported, return a null descriptor
        return VK_NULL_HANDLE;
    }
    if (is_null) {
        // Null buffer not supported, adjust offset and size
        offset = 0;
        size = 0;
    }
    const auto it{std::ranges::find_if(views, [offset, size, format](const BufferView& view) {
        return offset == view.offset && size == view.size && format == view.format;
    })};
    if (it != views.end()) {
        return *it->handle;
    }
    views.push_back({
        .offset = offset,
        .size = size,
        .format = format,
        .handle = device->logical().CreateBufferView({
            {},
            *buffer,
            device->surfaceFormat(FormatType::Buffer, false, format).format,
            offset,
            size,
        }),
    });
    return *views.back().handle;
}

BufferCacheRuntime::BufferCacheRuntime(const Device& device_, MemoryAllocator& memory_allocator_,
                                       scheduler::Scheduler& scheduler_,
                                       StagingBufferPool& staging_pool_,
                                       GuestDescriptorQueue& guest_descriptor_queue_,
                                       ComputePassDescriptorQueue& compute_pass_descriptor_queue)
    : device{device_},
      memory_allocator{memory_allocator_},
      scheduler{scheduler_},
      staging_pool{staging_pool_},
      guest_descriptor_queue{guest_descriptor_queue_} {
    const u32 ubo_align =
        static_cast<u32>(device.GetUniformBufferAlignment()  // check if the device has it
        );
    // add the ability to change the size in settings in future
    uniform_ring.Init(device, memory_allocator, 8 * 1024 * 1024 /* 8 MiB */,
                      ubo_align ? ubo_align : 256);
}

auto BufferCacheRuntime::UploadStagingBuffer(size_t size) -> StagingBufferRef {
    return staging_pool.Request(size, MemoryUsage::Upload);
}

auto BufferCacheRuntime::DownloadStagingBuffer(size_t size, bool deferred) -> StagingBufferRef {
    return staging_pool.Request(size, MemoryUsage::Download, deferred);
}
void BufferCacheRuntime::FreeDeferredStagingBuffer(StagingBufferRef& ref) {
    staging_pool.FreeDeferred(ref);
}

auto BufferCacheRuntime::GetDeviceLocalMemory() const -> u64 {
    return device.getDeviceLocalMemory();
}

auto BufferCacheRuntime::GetDeviceMemoryUsage() const -> u64 {
    return device.getDeviceMemoryUsage();
}

auto BufferCacheRuntime::CanReportMemoryUsage() const -> bool {
    return device.canReportMemoryUsage();
}

auto BufferCacheRuntime::GetStorageBufferAlignment() const -> u32 {
    return static_cast<u32>(device.GetStorageBufferAlignment());
}

void BufferCacheRuntime::TickFrame() noexcept { uniform_ring.BeginFrame(); }

void BufferCacheRuntime::Finish() { scheduler.finish(); }

void BufferCacheRuntime::CopyBuffer(vk::Buffer dst_buffer, vk::Buffer src_buffer,
                                    std::span<const texture::BufferCopy> copies, bool barrier,
                                    bool can_reorder_upload) {
    if (dst_buffer == VK_NULL_HANDLE || src_buffer == VK_NULL_HANDLE) {
        return;
    }
    static constexpr vk::MemoryBarrier READ_BARRIER{
        vk::AccessFlagBits::eMemoryWrite,
        vk::AccessFlagBits::eTransferRead | vk::AccessFlagBits::eTransferWrite};
    static constexpr vk::MemoryBarrier WRITE_BARRIER{
        vk::AccessFlagBits::eTransferWrite,
        vk::AccessFlagBits::eMemoryRead | vk::AccessFlagBits::eMemoryWrite,
    };

    // Measuring a popular game, this number never exceeds the specified size once data is warmed up
    boost::container::small_vector<vk::BufferCopy, 8> vk_copies(copies.size());
    std::ranges::transform(copies, vk_copies.begin(), MakeBufferCopy);
    if (src_buffer == staging_pool.StreamBuf() && can_reorder_upload) {
        scheduler.recordWithUploadBuffer([src_buffer, dst_buffer, vk_copies](
                                             vk::CommandBuffer, vk::CommandBuffer upload_cmdbuf) {
            upload_cmdbuf.copyBuffer(src_buffer, dst_buffer, vk_copies);
        });
        return;
    }

    scheduler.requestOutsideRenderOperationContext();
    scheduler.record([src_buffer, dst_buffer, vk_copies, barrier](vk::CommandBuffer cmdbuf) {
        if (barrier) {
            cmdbuf.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands,
                                   vk::PipelineStageFlagBits::eTransfer, {}, READ_BARRIER, {}, {});
        }
        cmdbuf.copyBuffer(src_buffer, dst_buffer, vk_copies);
        if (barrier) {
            cmdbuf.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                                   vk::PipelineStageFlagBits::eAllCommands, {}, WRITE_BARRIER, {},
                                   {});
        }
    });
}

void BufferCacheRuntime::ClearBuffer(vk::Buffer dest_buffer, u32 offset, size_t size, u32 value) {
    if (dest_buffer == VK_NULL_HANDLE) {
        return;
    }
    static constexpr vk::MemoryBarrier READ_BARRIER{
        vk::AccessFlagBits::eMemoryWrite,
        vk::AccessFlagBits::eTransferWrite | vk::AccessFlagBits::eTransferRead,
    };
    static constexpr vk::MemoryBarrier WRITE_BARRIER{
        vk::AccessFlagBits::eTransferWrite,
        vk::AccessFlagBits::eMemoryWrite | vk::AccessFlagBits::eMemoryRead,
    };

    scheduler.requestOutsideRenderOperationContext();
    scheduler.record([dest_buffer, offset, size, value](vk::CommandBuffer cmdbuf) -> void {
        cmdbuf.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands,
                               vk::PipelineStageFlagBits::eTransfer, {}, READ_BARRIER, {}, {});
        cmdbuf.fillBuffer(dest_buffer, offset, size, value);
        cmdbuf.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                               vk::PipelineStageFlagBits::eAllCommands, {}, WRITE_BARRIER, {}, {});
    });
}

void BufferCacheRuntime::BindIndexBuffer(IndexFormat index_format, vk::Buffer buffer) {
    vk::IndexType vk_index_type = ToIndexFormat(index_format);
    vk::Buffer vk_buffer = buffer;
    if (vk_buffer == VK_NULL_HANDLE) {
        // Vulkan doesn't support null index buffers. Replace it with our own null buffer.
        ReserveNullBuffer();
        vk_buffer = *null_buffer;
    }
    scheduler.record([vk_buffer, vk_index_type](vk::CommandBuffer cmdbuf) {
        cmdbuf.bindIndexBuffer(vk_buffer, 0, vk_index_type);
    });
}

void BufferCacheRuntime::BindVertexBuffer(u32 index, vk::Buffer buffer, u32 offset, u32 size,
                                          u32 stride) {
    if (index >= device.getMaxVertexInputBindings()) {
        return;
    }
    if (device.IsExtExtendedDynamicStateSupported()) {
        scheduler.record([index, buffer, offset, size, stride](vk::CommandBuffer cmdbuf) {
            const vk::DeviceSize vk_offset = buffer != VK_NULL_HANDLE ? offset : 0;
            const vk::DeviceSize vk_size = buffer != VK_NULL_HANDLE ? size : VK_WHOLE_SIZE;
            const vk::DeviceSize vk_stride = stride;
            cmdbuf.bindVertexBuffers2EXT(index, 1, &buffer, &vk_offset, &vk_size, &vk_stride);
        });
    } else {
        if (!device.HasNullDescriptor() && buffer == VK_NULL_HANDLE) {
            ReserveNullBuffer();
            buffer = *null_buffer;
            offset = 0;
        }
        scheduler.record([index, buffer, offset](vk::CommandBuffer cmdbuf) {
            cmdbuf.bindVertexBuffers(index, buffer, offset);
        });
    }
}

void BufferCacheRuntime::UniformRing::Init(const Device& device, MemoryAllocator& alloc, u64 bytes,
                                           u32 alignment) {
    for (size_t i = 0; i < NUM_FRAMES; ++i) {
        VkBufferCreateInfo ci{
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .size = bytes,
            .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .queueFamilyIndexCount = 0,
            .pQueueFamilyIndices = nullptr,
        };
        buffers[i] = alloc.createBuffer(ci, MemoryUsage::Upload);
        mapped[i] = buffers[i].Mapped().data();
    }
    size = bytes;
    align = alignment ? alignment : 256;
    head = 0;
    current_frame = 0;
}

auto BufferCacheRuntime::UniformRing::Alloc(u32 bytes, u32& out_offset) -> std::span<u8> {
    const u64 aligned = common::AlignUp(head, static_cast<u64>(align));
    u64 end = aligned + bytes;

    if (end > size) {
        return {};  // Fallback to staging pool
    }

    out_offset = static_cast<u32>(aligned);
    head = end;
    return {mapped[current_frame] + out_offset, bytes};
}

void BufferCacheRuntime::BindVertexBuffers(
    std::unique_ptr<buffer::HostBindings<BaseBufferCache>>&& bindings) {
    auto bind_buffer = [this](buffer::HostBindings<BaseBufferCache>* binding)
        -> boost::container::small_vector<vk::Buffer, 32> {
        boost::container::small_vector<vk::Buffer, 32> buffer_handles;
        for (u32 i = 0; i < binding->max_index; ++i) {
            auto handle = binding->buffers[i]->Handle();
            if (handle == VK_NULL_HANDLE) {
                binding->offsets[i] = 0;
                binding->sizes[i] = VK_WHOLE_SIZE;
                if (!device.HasNullDescriptor()) {
                    ReserveNullBuffer();
                    handle = *null_buffer;
                }
            }
            buffer_handles.push_back(handle);
        }
        return buffer_handles;
    };

    const u32 device_max = device.getMaxVertexInputBindings();
    const u32 min_binding = std::min(bindings->min_index, device_max);
    const u32 max_binding = std::min(bindings->max_index, device_max);
    const u32 binding_count = max_binding - min_binding;
    if (binding_count == 0) {
        return;
    }
    if (device.IsExtExtendedDynamicStateSupported()) {
        scheduler.record([bindings_ = std::move(bindings), bind_buffer,
                          binding_count](vk::CommandBuffer cmdbuf) {
            auto buffer_handle = bind_buffer(bindings_.get());
            cmdbuf.bindVertexBuffers2EXT(bindings_->min_index, binding_count, buffer_handle.data(),
                                         bindings_->offsets.data(), bindings_->sizes.data(),
                                         bindings_->strides.data());
        });
    } else {
        scheduler.record([bindings_ = std::move(bindings), bind_buffer,
                          binding_count](vk::CommandBuffer cmdbuf) {
            auto buffer_handle = bind_buffer(bindings_.get());
            cmdbuf.bindVertexBuffers(bindings_->min_index, binding_count, buffer_handle.data(),
                                     bindings_->offsets.data());
        });
    }
}

void BufferCacheRuntime::BindTransformFeedbackBuffer(u32 index, vk::Buffer buffer, u32 offset,
                                                     u32 size) {
    if (!device.IsExtTransformFeedbackSupported()) {
        // Already logged in the rasterizer
        return;
    }
    if (buffer == VK_NULL_HANDLE) {
        // Vulkan doesn't support null transform feedback buffers.
        // Replace it with our own null buffer.
        ReserveNullBuffer();
        buffer = *null_buffer;
        offset = 0;
        size = 0;
    }
    scheduler.record([index, buffer, offset, size](vk::CommandBuffer cmdbuf) {
        const vk::DeviceSize vk_offset = offset;
        const vk::DeviceSize vk_size = size;
        cmdbuf.bindTransformFeedbackBuffersEXT(index, 1, &buffer, &vk_offset, &vk_size);
    });
}

void BufferCacheRuntime::BindTransformFeedbackBuffers(
    buffer::HostBindings<BaseBufferCache>& bindings) {
    if (!device.IsExtTransformFeedbackSupported()) {
        // Already logged in the rasterizer
        return;
    }
    boost::container::small_vector<vk::Buffer, 4> buffer_handles;
    for (u32 index = 0; index < bindings.buffers.size(); ++index) {
        buffer_handles.push_back(bindings.buffers[index]->Handle());
    }
    scheduler.record([bindings_ = std::move(bindings),
                      buffer_handles_ = std::move(buffer_handles)](vk::CommandBuffer cmdbuf) {
        cmdbuf.bindTransformFeedbackBuffersEXT(0, static_cast<u32>(buffer_handles_.size()),
                                               buffer_handles_.data(), bindings_.offsets.data(),
                                               bindings_.sizes.data());
    });
}

void BufferCacheRuntime::ReserveNullBuffer() {
    if (!null_buffer) {
        null_buffer = CreateNullBuffer();
    }
}

auto BufferCacheRuntime::CreateNullBuffer() -> Buffer {
    VkBufferCreateInfo create_info{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = 4,
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
    };
    if (device.IsExtTransformFeedbackSupported()) {
        create_info.usage |= VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_BUFFER_BIT_EXT;
    }
    Buffer ret = memory_allocator.createBuffer(create_info, MemoryUsage::DeviceLocal);
    if (device.hasDebuggingToolAttached()) {
        ret.SetObjectNameEXT("Null buffer");
    }

    scheduler.requestOutsideRenderOperationContext();
    scheduler.record([buffer = *ret](vk::CommandBuffer cmdbuf) {
        cmdbuf.fillBuffer(buffer, 0, VK_WHOLE_SIZE, 0);
    });

    return ret;
}

}  // namespace render::vulkan