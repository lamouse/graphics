#include "layer.hpp"
#include "common/scope_exit.h"
#include "fsr.hpp"
#include "anti_alias_pass.h"
#include "present_push_constants.h"
#include "render_vulkan/blit_screen.hpp"
#include "vulkan_common/device.hpp"
#include "vulkan_common/vulkan_wrapper.hpp"
#include "vulkan_common/memory_allocator.hpp"
#include "render_vulkan/scheduler.hpp"
#include "filters.hpp"
#include "common/settings.hpp"
#include "vulkan_utils.hpp"
#include "render_core/surface.hpp"
#include "common/alignment.h"
#include "render_core/render_vulkan/vk_graphic.hpp"

namespace render::vulkan {
namespace {

auto GetBytesPerPixel() -> u32 {
    using namespace surface;
    return BytesPerBlock(PixelFormat::A8B8G8R8_UNORM);
}
auto GetSizeInBytes(const frame::FramebufferConfig& framebuffer) -> std::size_t {
    return static_cast<std::size_t>(common::AlignUpLog2(framebuffer.width, framebuffer.stride)) *
           static_cast<std::size_t>(framebuffer.height) * GetBytesPerPixel();
}

}  // namespace
Layer::Layer(const Device& device_, MemoryAllocator& memory_allocator_,
             scheduler::Scheduler& scheduler_, size_t image_count_, vk::Extent2D output_size,
             vk::DescriptorSetLayout layout)
    : device(device_),
      memory_allocator(memory_allocator_),
      scheduler(scheduler_),
      image_count(image_count_) {
    CreateDescriptorPool();
    CreateDescriptorSets(layout);
    if (common::settings::get<settings::Graphics>().scaling_filter ==
        settings::enums::ScalingFilter::Fsr) {
        CreateFSR(output_size);
    }
}
Layer::~Layer() { ReleaseRawImages(); }

void Layer::CreateDescriptorPool() {
    descriptor_pool = present::utils::CreateWrappedDescriptorPool(device, image_count, image_count);
}

void Layer::CreateDescriptorSets(vk::DescriptorSetLayout layout) {
    const std::vector layouts(image_count, layout);
    descriptor_sets = present::utils::CreateWrappedDescriptorSets(descriptor_pool, layouts);
}

void Layer::CreateStagingBuffer(const frame::FramebufferConfig& framebuffer) {
    const VkBufferCreateInfo ci{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = CalculateBufferSize(framebuffer),
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                 VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
    };

    buffer = memory_allocator.createBuffer(ci, MemoryUsage::Upload);
}

auto Layer::CalculateBufferSize(const frame::FramebufferConfig& framebuffer) const -> u64 {
    return GetSizeInBytes(framebuffer) * image_count;
}

void Layer::CreateRawImages(const frame::FramebufferConfig& framebuffer) {
    const auto format = vk::Format::eA8B8G8R8UnormPack32;  // TODO 有时间处理
    resource_ticks.resize(image_count);
    raw_images.resize(image_count);
    raw_image_views.resize(image_count);

    for (size_t i = 0; i < image_count; ++i) {
        raw_images[i] = present::utils::CreateWrappedImage(
            memory_allocator, {framebuffer.width, framebuffer.height}, format);
        raw_image_views[i] = present::utils::CreateWrappedImageView(device, raw_images[i], format);
    }
}
void Layer::CreateFSR(vk::Extent2D output_size) {
    fsr = std::make_unique<FSR>(device, memory_allocator, image_count, output_size);
}

void Layer::RefreshResources(const frame::FramebufferConfig& framebuffer) {
    if (framebuffer.width == raw_width && framebuffer.height == raw_height && !raw_images.empty()) {
        return;
    }

    raw_width = framebuffer.width;
    raw_height = framebuffer.height;
    anti_alias.reset();

    ReleaseRawImages();
    CreateStagingBuffer(framebuffer);
    CreateRawImages(framebuffer);
}

void Layer::SetAntiAliasPass() {
    if (anti_alias) {
        return;
    }
    anti_alias = std::make_unique<NoAA>();

    // TODO 待完成

    // anti_alias_setting = filters.get_anti_aliasing();
    // const VkExtent2D render_area{
    //     .width = Settings::values.resolution_info.ScaleUp(raw_width),
    //     .height = Settings::values.resolution_info.ScaleUp(raw_height),
    // };

    // switch (anti_alias_setting) {
    // case Settings::AntiAliasing::Fxaa:
    //     anti_alias = std::make_unique<FXAA>(device, memory_allocator, image_count, render_area);
    //     break;
    // case Settings::AntiAliasing::Smaa:
    //     anti_alias = std::make_unique<SMAA>(device, memory_allocator, image_count, render_area);
    //     break;
    // default:

    //     break;
    // }
}

void Layer::ReleaseRawImages() {
    for (const u64 tick : resource_ticks) {
        scheduler.wait(tick);
    }
    raw_images.clear();
    buffer.reset();
}

auto Layer::GetRawImageOffset(const frame::FramebufferConfig& framebuffer, size_t image_index) const
    -> u64 {
    return GetSizeInBytes(framebuffer) * image_index;
}

void Layer::SetMatrixData(PresentPushConstants& data,
                          const layout::FrameBufferLayout& layout) const {
    data.model_view_matrix =
        MakeOrthographicMatrix(static_cast<f32>(layout.width), static_cast<f32>(layout.height));
}

void Layer::SetVertexData(PresentPushConstants& data, const layout::FrameBufferLayout& layout,
                          const common::Rectangle<f32>& crop) const {
    // Map the coordinates to the screen.
    const auto& screen = layout.screen;
    const auto x = static_cast<f32>(screen.left);
    const auto y = static_cast<f32>(screen.top);
    const auto w = static_cast<f32>(screen.GetWidth());
    const auto h = static_cast<f32>(screen.GetHeight());

    data.vertices[0] = ScreenRectVertex(x, y, crop.left, crop.top);
    data.vertices[1] = ScreenRectVertex(x + w, y, crop.right, crop.top);
    data.vertices[2] = ScreenRectVertex(x, y + h, crop.left, crop.bottom);
    data.vertices[3] = ScreenRectVertex(x + w, y + h, crop.right, crop.bottom);
}

void Layer::UpdateDescriptorSet(vk::ImageView image_view, vk::Sampler sampler, size_t image_index) {
    const vk::DescriptorImageInfo image_info = vk::DescriptorImageInfo()
                                                   .setSampler(sampler)
                                                   .setImageView(image_view)
                                                   .setImageLayout(vk::ImageLayout::eGeneral);
    const vk::WriteDescriptorSet write =
        vk::WriteDescriptorSet()
            .setDstSet(descriptor_sets[image_index])
            .setDstBinding(0)
            .setDstArrayElement(0)
            .setDescriptorCount(1)
            .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
            .setPImageInfo(&image_info);
    device.getLogical().updateDescriptorSets(write, {});
}

void Layer::ConfigureDraw(PresentPushConstants* out_push_constants,
                          vk::DescriptorSet* out_descriptor_set, VulkanGraphics& rasterizer,
                          vk::Sampler sampler, size_t image_index,
                          const frame::FramebufferConfig& framebuffer,
                          const layout::FrameBufferLayout& layout) {
    const auto texture_info = rasterizer.AccelerateDisplay(framebuffer, framebuffer.stride);

    const u32 texture_width = texture_info ? texture_info->width : framebuffer.width;
    ;
    const u32 texture_height = texture_info ? texture_info->height : framebuffer.height;
    const u32 scaled_width = texture_info ? texture_info->scaled_width : texture_width;
    const u32 scaled_height = texture_info ? texture_info->scaled_height : texture_height;
    RefreshResources(framebuffer);
    SetAntiAliasPass();

    // Finish any pending render pass
    scheduler.requestOutsideRenderPassOperationContext();
    scheduler.wait(resource_ticks[image_index]);
    SCOPE_EXIT -> void { resource_ticks[image_index] = scheduler.currentTick(); };

    vk::Image source_image = texture_info ? texture_info->image : *raw_images[image_index];
    vk::ImageView source_image_view =
        texture_info ? texture_info->image_view : *raw_image_views[image_index];
    // 这里什么也么做
    anti_alias->Draw(scheduler, image_index, &source_image, &source_image_view);
    auto crop_rect = frame::NormalizeCrop(framebuffer, texture_width, texture_height);
    const vk::Extent2D render_extent{
        scaled_width,
        scaled_height,
    };

    if (fsr) {
        source_image_view = fsr->Draw(scheduler, image_index, source_image, source_image_view,
                                      render_extent, crop_rect);
        crop_rect = {0, 0, 1, 1};
    }
    SetMatrixData(*out_push_constants, layout);
    // 这里也需要处理
    SetVertexData(*out_push_constants, layout, crop_rect);

    UpdateDescriptorSet(source_image_view, sampler, image_index);
    *out_descriptor_set = descriptor_sets[image_index];
}
}  // namespace render::vulkan