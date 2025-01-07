#include "vk_rasterizer.hpp"
#include "common/microprofile.hpp"
namespace render::vulkan {
MICROPROFILE_DEFINE(Vulkan_WaitForWorker, "Vulkan", "Wait for worker", MP_RGB(255, 192, 192));
MICROPROFILE_DEFINE(Vulkan_Drawing, "Vulkan", "Record drawing", MP_RGB(192, 128, 128));
MICROPROFILE_DEFINE(Vulkan_Compute, "Vulkan", "Record compute", MP_RGB(192, 128, 128));
MICROPROFILE_DEFINE(Vulkan_Clearing, "Vulkan", "Record clearing", MP_RGB(192, 128, 128));
MICROPROFILE_DEFINE(Vulkan_PipelineCache, "Vulkan", "Pipeline cache", MP_RGB(192, 128, 128));
RasterizerVulkan::RasterizerVulkan(core::frontend::BaseWindow& emu_window_, const Device& device_,
                                   MemoryAllocator& memory_allocator_,
                                   scheduler::Scheduler& scheduler_)
    : device{device_},
      memory_allocator{memory_allocator_},
      scheduler{scheduler_},
      blit_image(device_, scheduler, descriptor_pool),
      staging_pool(device, memory_allocator, scheduler),
      descriptor_pool(device, scheduler),
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
      wfi_event(device.getLogical().createEvent({})) {}

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
}  // namespace render::vulkan