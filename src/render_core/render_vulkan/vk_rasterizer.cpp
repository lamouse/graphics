#include "vk_rasterizer.hpp"
#include <spdlog/spdlog.h>
#include "common/microprofile.hpp"
#include "common/scope_exit.h"
#include "blit_screen.hpp"
namespace render::vulkan {

namespace {
struct DrawParams {
        u32 base_instance;
        u32 num_instances;
        u32 base_vertex;
        u32 num_vertices;
        u32 first_index;
        bool is_indexed;
};

}  // namespace

MICROPROFILE_DEFINE(Vulkan_WaitForWorker, "Vulkan", "Wait for worker", MP_RGB(255, 192, 192));
MICROPROFILE_DEFINE(Vulkan_Drawing, "Vulkan", "Record drawing", MP_RGB(192, 128, 128));
MICROPROFILE_DEFINE(Vulkan_Compute, "Vulkan", "Record compute", MP_RGB(192, 128, 128));
MICROPROFILE_DEFINE(Vulkan_Clearing, "Vulkan", "Record clearing", MP_RGB(192, 128, 128));
MICROPROFILE_DEFINE(Vulkan_PipelineCache, "Vulkan", "Pipeline cache", MP_RGB(192, 128, 128));
RasterizerVulkan::RasterizerVulkan(const Device& device_, MemoryAllocator& memory_allocator_,
                                   scheduler::Scheduler& scheduler_)
    : device{device_},
      memory_allocator{memory_allocator_},
      scheduler{scheduler_},
      staging_pool(device, memory_allocator, scheduler),
      descriptor_pool(device, scheduler),
      blit_image(device_, scheduler, descriptor_pool),
      guest_descriptor_queue(device, scheduler),
      compute_pass_descriptor_queue(device, scheduler),
      render_pass_cache(device),
      texture_cache_runtime{device,
                            scheduler,
                            memory_allocator,
                            staging_pool,
                            render_pass_cache,
                            descriptor_pool,
                            compute_pass_descriptor_queue,
                            blit_image},
      wfi_event(device.logical().createEvent({})) {}

template <typename Func>
void RasterizerVulkan::PrepareDraw(bool is_indexed, Func&& draw_func) {
    MICROPROFILE_SCOPE(Vulkan_Drawing);

    FlushWork();

    // GraphicsPipeline* const pipeline{pipeline_cache.CurrentGraphicsPipeline()};
    //  if (!pipeline) {
    //      return;
    //  }
    //  std::scoped_lock lock{buffer_cache.mutex, texture_cache.mutex};
    //  update engine as channel may be different.

    UpdateDynamicStates();

    HandleTransformFeedback();
    draw_func();
}

void RasterizerVulkan::Draw(bool is_indexed, u32 instance_count) {
    PrepareDraw(is_indexed, [this, is_indexed, instance_count] {
        const DrawParams draw_params{};

        // TODO 修改
        scheduler.record([draw_params](vk::CommandBuffer cmdbuf) {
            if (draw_params.is_indexed) {
                cmdbuf.drawIndexed(draw_params.num_vertices, draw_params.num_instances,
                                   draw_params.first_index, draw_params.base_vertex,
                                   draw_params.base_instance);
            } else {
                cmdbuf.draw(draw_params.num_vertices, draw_params.num_instances,
                            draw_params.base_vertex, draw_params.base_instance);
            }
        });
    });
}

void RasterizerVulkan::UpdateDynamicStates() {
    // TODO 暂时为空
}

void RasterizerVulkan::DrawTexture() {
    MICROPROFILE_SCOPE(Vulkan_Drawing);

    FlushWork();

    UpdateDynamicStates();
    // TODO 这里需要完成
}

void RasterizerVulkan::Clear(u32 layer_count) {
    MICROPROFILE_SCOPE(Vulkan_Clearing);

    FlushWork();
}

void RasterizerVulkan::DrawIndirect() {
    // const auto& params = maxwell3d->draw_manager->GetIndirectParams();
    // buffer_cache.SetDrawIndirect(&params);
    // PrepareDraw(params.is_indexed, [this, &params] {
    //     const auto indirect_buffer = buffer_cache.GetDrawIndirectBuffer();
    //     const auto& buffer = indirect_buffer.first;
    //     const auto& offset = indirect_buffer.second;
    //     if (params.is_byte_count) {
    //         scheduler.Record([buffer_obj = buffer->Handle(), offset,
    //                           stride = params.stride](vk::CommandBuffer cmdbuf) {
    //             cmdbuf.DrawIndirectByteCountEXT(1, 0, buffer_obj, offset, 0,
    //                                             static_cast<u32>(stride));
    //         });
    //         return;
    //     }
    //     if (params.include_count) {
    //         const auto count = buffer_cache.GetDrawIndirectCount();
    //         const auto& draw_buffer = count.first;
    //         const auto& offset_base = count.second;
    //         scheduler.Record([draw_buffer_obj = draw_buffer->Handle(),
    //                           buffer_obj = buffer->Handle(), offset_base, offset,
    //                           params](vk::CommandBuffer cmdbuf) {
    //             if (params.is_indexed) {
    //                 cmdbuf.DrawIndexedIndirectCount(
    //                     buffer_obj, offset, draw_buffer_obj, offset_base,
    //                     static_cast<u32>(params.max_draw_counts),
    //                     static_cast<u32>(params.stride));
    //             } else {
    //                 cmdbuf.DrawIndirectCount(buffer_obj, offset, draw_buffer_obj, offset_base,
    //                                          static_cast<u32>(params.max_draw_counts),
    //                                          static_cast<u32>(params.stride));
    //             }
    //         });
    //         return;
    //     }
    //     scheduler.Record([buffer_obj = buffer->Handle(), offset, params](vk::CommandBuffer
    //     cmdbuf) {
    //         if (params.is_indexed) {
    //             cmdbuf.DrawIndexedIndirect(buffer_obj, offset,
    //                                        static_cast<u32>(params.max_draw_counts),
    //                                        static_cast<u32>(params.stride));
    //         } else {
    //             cmdbuf.DrawIndirect(buffer_obj, offset, static_cast<u32>(params.max_draw_counts),
    //                                 static_cast<u32>(params.stride));
    //         }
    //     });
    // });
    // buffer_cache.SetDrawIndirect(nullptr);
}

void RasterizerVulkan::DispatchCompute() {
    FlushWork();

    // ComputePipeline* const pipeline{pipeline_cache.CurrentComputePipeline()};
    // if (!pipeline) {
    //     return;
    // }
    // std::scoped_lock lock{texture_cache.mutex, buffer_cache.mutex};
    // pipeline->Configure(*kepler_compute, *gpu_memory, scheduler, buffer_cache, texture_cache);

    // const std::array<u32, 3> dim{qmd.grid_dim_x, qmd.grid_dim_y, qmd.grid_dim_z};
    // scheduler.requestOutsideRenderPassOperationContext();
    // scheduler.record([dim](vk::CommandBuffer cmdbuf) { cmdbuf.ddispatch(dim[0], dim[1], dim[2]);
    // });
}

void RasterizerVulkan::BindGraphicsUniformBuffer(size_t stage, u32 index, GPUVAddr gpu_addr,
                                                 u32 size) {
    // buffer_cache.BindGraphicsUniformBuffer(stage, index, gpu_addr, size);
}

void RasterizerVulkan::DisableGraphicsUniformBuffer(size_t stage, u32 index) {
    // buffer_cache.DisableGraphicsUniformBuffer(stage, index);
}
void RasterizerVulkan::FlushAll() {}

void RasterizerVulkan::FlushRegion(DAddr addr, u64 size, CacheType which) {
    if (addr == 0 || size == 0) {
        return;
    }
    if (True(which & CacheType::TextureCache)) {
        // std::scoped_lock lock{texture_cache.mutex};
        // texture_cache.DownloadMemory(addr, size);
    }
    if ((True(which & CacheType::BufferCache))) {
        // std::scoped_lock lock{buffer_cache.mutex};
        // buffer_cache.DownloadMemory(addr, size);
    }
    if ((True(which & CacheType::QueryCache))) {
        // query_cache.FlushRegion(addr, size);
    }
}

bool RasterizerVulkan::MustFlushRegion(DAddr addr, u64 size, CacheType which) {
    if ((True(which & CacheType::BufferCache))) {
        // std::scoped_lock lock{buffer_cache.mutex};
        // if (buffer_cache.IsRegionGpuModified(addr, size)) {
        //     return true;
        // }
    }
    // if (!Settings::IsGPULevelHigh()) {
    //     return false;
    // }
    if (True(which & CacheType::TextureCache)) {
        // std::scoped_lock lock{texture_cache.mutex};
        // return texture_cache.IsRegionGpuModified(addr, size);
    }
    return false;
}

auto RasterizerVulkan::GetFlushArea(DAddr addr, u64 size) -> RasterizerDownloadArea {
    {
        // std::scoped_lock lock{texture_cache.mutex};
        // auto area = texture_cache.GetFlushArea(addr, size);
        // if (area) {
        //     return *area;
        // }
    }
    RasterizerDownloadArea new_area{
        // .start_address = Common::AlignDown(addr, Core::DEVICE_PAGESIZE),
        // .end_address = Common::AlignUp(addr + size, Core::DEVICE_PAGESIZE),
        // .preemtive = true,
    };
    return new_area;
}

void RasterizerVulkan::InvalidateRegion(DAddr addr, u64 size, CacheType which) {
    if (addr == 0 || size == 0) {
        return;
    }
    if (True(which & CacheType::TextureCache)) {
        // std::scoped_lock lock{texture_cache.mutex};
        // texture_cache.WriteMemory(addr, size);
    }
    if ((True(which & CacheType::BufferCache))) {
        // std::scoped_lock lock{buffer_cache.mutex};
        // buffer_cache.WriteMemory(addr, size);
    }
    if ((True(which & CacheType::QueryCache))) {
        // query_cache.InvalidateRegion(addr, size);
    }
    if ((True(which & CacheType::ShaderCache))) {
        // pipeline_cache.InvalidateRegion(addr, size);
    }
}

void RasterizerVulkan::InnerInvalidation(std::span<const std::pair<DAddr, std::size_t>> sequences) {
    // {
    //     std::scoped_lock lock{texture_cache.mutex};
    //     for (const auto& [addr, size] : sequences) {
    //         texture_cache.WriteMemory(addr, size);
    //     }
    // }
    // {
    //     std::scoped_lock lock{buffer_cache.mutex};
    //     for (const auto& [addr, size] : sequences) {
    //         buffer_cache.WriteMemory(addr, size);
    //     }
    // }
    // {
    //     for (const auto& [addr, size] : sequences) {
    //         query_cache.InvalidateRegion(addr, size);
    //         pipeline_cache.InvalidateRegion(addr, size);
    //     }
    // }
}

void RasterizerVulkan::OnCacheInvalidation(DAddr addr, u64 size) {
    if (addr == 0 || size == 0) {
        return;
    }

    // {
    //     std::scoped_lock lock{texture_cache.mutex};
    //     texture_cache.WriteMemory(addr, size);
    // }
    // {
    //     std::scoped_lock lock{buffer_cache.mutex};
    //     buffer_cache.WriteMemory(addr, size);
    // }
    // pipeline_cache.InvalidateRegion(addr, size);
}

bool RasterizerVulkan::OnCPUWrite(DAddr addr, u64 size) {
    if (addr == 0 || size == 0) {
        return false;
    }

    // {
    //     std::scoped_lock lock{buffer_cache.mutex};
    //     if (buffer_cache.OnCPUWrite(addr, size)) {
    //         return true;
    //     }
    // }

    // {
    //     std::scoped_lock lock{texture_cache.mutex};
    //     texture_cache.WriteMemory(addr, size);
    // }

    // pipeline_cache.InvalidateRegion(addr, size);
    return false;
}

void RasterizerVulkan::InvalidateGPUCache() {}

void RasterizerVulkan::UnmapMemory(DAddr addr, u64 size) {
    // {
    //     std::scoped_lock lock{texture_cache.mutex};
    //     texture_cache.UnmapMemory(addr, size);
    // }
    // {
    //     std::scoped_lock lock{buffer_cache.mutex};
    //     buffer_cache.WriteMemory(addr, size);
    // }
    // pipeline_cache.OnCacheInvalidation(addr, size);
}

void RasterizerVulkan::ModifyGPUMemory(size_t as_id, GPUVAddr addr, u64 size) {
    // {
    //     std::scoped_lock lock{texture_cache.mutex};
    //     texture_cache.UnmapGPUMemory(as_id, addr, size);
    // }
}

void RasterizerVulkan::SignalFence(std::function<void()>&& func) {
    // fence_manager.SignalFence(std::move(func));
}

void RasterizerVulkan::SyncOperation(std::function<void()>&& func) {
    // fence_manager.SyncOperation(std::move(func));
}
void RasterizerVulkan::SignalSyncPoint(u32 value) {
    // fence_manager.SignalSyncPoint(value);
}

void RasterizerVulkan::SignalReference() {
    // fence_manager.SignalReference();
}

void RasterizerVulkan::ReleaseFences(bool force) {
    // fence_manager.WaitPendingFences(force);
}

void RasterizerVulkan::FlushAndInvalidateRegion(DAddr addr, u64 size, CacheType which) {
    // if (Settings::IsGPULevelExtreme()) {
    //     FlushRegion(addr, size, which);
    // }
    InvalidateRegion(addr, size, which);
}

void RasterizerVulkan::WaitForIdle() {
    // Everything but wait pixel operations. This intentionally includes FRAGMENT_SHADER_BIT because
    // fragment shaders can still write storage buffers.
    vk::PipelineStageFlags flags =
        vk::PipelineStageFlagBits::eDrawIndirect | vk::PipelineStageFlagBits::eVertexInput |
        vk::PipelineStageFlagBits::eVertexShader |
        vk::PipelineStageFlagBits::eTessellationControlShader |
        vk::PipelineStageFlagBits::eTessellationEvaluationShader |
        vk::PipelineStageFlagBits::eGeometryShader | vk::PipelineStageFlagBits::eFragmentShader |
        vk::PipelineStageFlagBits::eComputeShader | vk::PipelineStageFlagBits::eTransfer;
    if (device.isExtTransformFeedbackSupported()) {
        flags |= vk::PipelineStageFlagBits::eTransformFeedbackEXT;
    }

    scheduler.requestOutsideRenderPassOperationContext();
    scheduler.record([event = *wfi_event, flags](vk::CommandBuffer cmdbuf) {
        cmdbuf.setEvent(event, flags);
        cmdbuf.waitEvents(event, flags, vk::PipelineStageFlagBits::eTopOfPipe, {}, {}, {});
    });
    // fence_manager.SignalOrdering();
}

void RasterizerVulkan::FragmentBarrier() {
    // We already put barriers when a render pass finishes
    scheduler.requestOutsideRenderPassOperationContext();
}

void RasterizerVulkan::TiledCacheBarrier() {
    // TODO: Implementing tiled barriers requires rewriting a good chunk of the Vulkan backend
}

void RasterizerVulkan::FlushCommands() {
    if (draw_counter == 0) {
        return;
    }
    draw_counter = 0;
    scheduler.flush();
}

void RasterizerVulkan::TickFrame() {
    draw_counter = 0;
    // guest_descriptor_queue.TickFrame();
    // compute_pass_descriptor_queue.TickFrame();
    // fence_manager.TickFrame();
    // staging_pool.TickFrame();
    // {
    //     std::scoped_lock lock{texture_cache.mutex};
    //     texture_cache.TickFrame();
    // }
    // {
    //     std::scoped_lock lock{buffer_cache.mutex};
    //     buffer_cache.TickFrame();
    // }
}

auto RasterizerVulkan::AccelerateConditionalRendering() -> bool { return true; }

void RasterizerVulkan::AccelerateInlineToMemory(GPUVAddr address, size_t copy_size,
                                                std::span<const u8> memory) {
    // auto cpu_addr = gpu_memory->GpuToCpuAddress(address);
    // if (!cpu_addr) [[unlikely]] {
    //     gpu_memory->WriteBlock(address, memory.data(), copy_size);
    //     return;
    // }
    // gpu_memory->WriteBlockUnsafe(address, memory.data(), copy_size);
    // {
    //     std::unique_lock<std::recursive_mutex> lock{buffer_cache.mutex};
    //     if (!buffer_cache.InlineMemory(*cpu_addr, copy_size, memory)) {
    //         buffer_cache.WriteMemory(*cpu_addr, copy_size);
    //     }
    // }
    // {
    //     std::scoped_lock lock_texture{texture_cache.mutex};
    //     texture_cache.WriteMemory(*cpu_addr, copy_size);
    // }
    // pipeline_cache.InvalidateRegion(*cpu_addr, copy_size);
    // query_cache.InvalidateRegion(*cpu_addr, copy_size);
}

void RasterizerVulkan::ReleaseChannel(s32 channel_id) {
    // EraseChannel(channel_id);
    // {
    //     std::scoped_lock lock{buffer_cache.mutex, texture_cache.mutex};
    //     texture_cache.EraseChannel(channel_id);
    //     buffer_cache.EraseChannel(channel_id);
    // }
    // pipeline_cache.EraseChannel(channel_id);
    // query_cache.EraseChannel(channel_id);
}

std::optional<FramebufferTextureInfo> RasterizerVulkan::AccelerateDisplay(
    const frame::FramebufferConfig& config, DAddr framebuffer_addr, u32 pixel_stride) {
    if (!framebuffer_addr) {
        return {};
    }
    // std::scoped_lock lock{texture_cache.mutex};
    // const auto [image_view, scaled] =
    // texture_cache.TryFindFramebufferImageView(config, framebuffer_addr);
    // if (!image_view) {
    //     return {};
    // }

    // const auto& resolution = Settings::values.resolution_info;

    // FramebufferTextureInfo info{};
    // info.image = image_view->ImageHandle();
    // info.image_view = image_view->Handle(Shader::TextureType::Color2D);
    // info.width = image_view->size.width;
    // info.height = image_view->size.height;
    // info.scaled_width = scaled ? resolution.ScaleUp(info.width) : info.width;
    // info.scaled_height = scaled ? resolution.ScaleUp(info.height) : info.height;
    // return info;

    return {};
}

void RasterizerVulkan::FlushWork() {
#ifdef ANDROID
    static constexpr u32 DRAWS_TO_DISPATCH = 1024;
#else
    static constexpr u32 DRAWS_TO_DISPATCH = 4096;
#endif  // ANDROID

    // Only check multiples of 8 draws
    static_assert(DRAWS_TO_DISPATCH % 8 == 0);
    if ((++draw_counter & 7) != 7) {
        return;
    }
    if (draw_counter < DRAWS_TO_DISPATCH) {
        // Send recorded tasks to the worker thread
        scheduler.dispatchWork();
        return;
    }
    // Otherwise (every certain number of draws) flush execution.
    // This submits commands to the Vulkan driver.
    scheduler.flush();
    draw_counter = 0;
}

void RasterizerVulkan::HandleTransformFeedback() {
    static std::once_flag warn_unsupported;

    if (!device.isExtTransformFeedbackSupported()) {
        std::call_once(warn_unsupported,
                       [&] { SPDLOG_ERROR("Transform feedbacks used but not supported"); });
        return;
    }
}

}  // namespace render::vulkan