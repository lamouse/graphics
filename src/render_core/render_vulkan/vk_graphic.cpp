#include "vk_graphic.hpp"
namespace render::vulkan {
VulkanGraphics::VulkanGraphics(core::frontend::BaseWindow* emu_window_, const Device& device_,
                               MemoryAllocator& memory_allocator_, scheduler::Scheduler& scheduler_)
    : device(device_),
      memory_allocator(memory_allocator_),
      scheduler(scheduler_),
      staging_pool(device, memory_allocator, scheduler),
      descriptor_pool(device, scheduler),
      guest_descriptor_queue(device, scheduler),
      compute_pass_descriptor_queue(device, scheduler),
      blit_image(device, scheduler, descriptor_pool),
      render_pass_cache(device),
      texture_cache_runtime{device,
                            scheduler,
                            memory_allocator,
                            staging_pool,
                            render_pass_cache,
                            descriptor_pool,
                            compute_pass_descriptor_queue,
                            blit_image},
      texture_cache(texture_cache_runtime),
      buffer_cache_runtime(device, memory_allocator, scheduler, staging_pool,
                           guest_descriptor_queue, compute_pass_descriptor_queue, descriptor_pool),
      buffer_cache(buffer_cache_runtime),
      pipeline_cache(device, scheduler, descriptor_pool, guest_descriptor_queue, render_pass_cache,
                     buffer_cache, texture_cache),
      wfi_event(device.logical().createEvent()) {}

VulkanGraphics::~VulkanGraphics() = default;
}  // namespace render::vulkan