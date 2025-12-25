#include "layer.hpp"
#include "common/scope_exit.h"
#include "fsr.hpp"
#include "anti_alias_pass.h"
#include "present_push_constants.h"
#include "render_vulkan/blit_screen.hpp"
#include "vulkan_common/memory_allocator.hpp"
#include "render_vulkan/scheduler.hpp"
#include "filters.hpp"
#include "common/settings.hpp"
#include "vulkan_utils.hpp"
#include "render_core/render_vulkan/vk_graphic.hpp"
import render.vulkan.common;

namespace render::vulkan {
Layer::Layer(const Device& device_, MemoryAllocator& memory_allocator_,
             scheduler::Scheduler& scheduler_, size_t image_count_, vk::Extent2D output_size,
             vk::DescriptorSetLayout layout)
    : device(device_),
      memory_allocator(memory_allocator_),
      scheduler(scheduler_),
      image_count(image_count_) {
    CreateDescriptorPool();
    CreateDescriptorSets(layout);
    if (settings::values.scaling_filter.GetValue() == settings::enums::ScalingFilter::Fsr) {
        CreateFSR(output_size);
    }
}
Layer::~Layer() { ReleaseRawImages(); }

void Layer::CreateDescriptorPool() {
    descriptor_pool = present::utils::CreateWrappedDescriptorPool(device, image_count, image_count);
}

void Layer::CreateDescriptorSets(vk::DescriptorSetLayout layout) {
    std::vector<vk::DescriptorSetLayout> layouts(image_count, layout);
    descriptor_sets = present::utils::CreateWrappedDescriptorSets(descriptor_pool, layouts);
}

void Layer::CreateRawImages(const frame::FramebufferConfig& framebuffer) {
    const auto format = vk::Format::eB8G8R8A8Unorm;  // TODO 有时间处理
    resource_ticks.resize(image_count);
    raw_images.resize(image_count);
    raw_image_views.resize(image_count);

    for (size_t i = 0; i < image_count; ++i) {
        raw_images[i] = present::utils::CreateWrappedImage(
            memory_allocator, {framebuffer.width, framebuffer.height}, format);
        scheduler.record([image = *raw_images[i]](vk::CommandBuffer cmdbuf) -> void {
            present::utils::TransitionImageLayout(cmdbuf, image, vk::ImageLayout::eGeneral,
                                                  vk::ImageLayout::eUndefined);
        });

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
    const u32 texture_height = texture_info ? texture_info->height : framebuffer.height;
    const u32 scaled_width = texture_info ? texture_info->scaled_width : texture_width;
    const u32 scaled_height = texture_info ? texture_info->scaled_height : texture_height;
    RefreshResources(framebuffer);
    SetAntiAliasPass();

    // Finish any pending render pass
    scheduler.requestOutsideRenderOperationContext();
    scheduler.wait(resource_ticks[image_index]);
    SCOPE_EXIT->void { resource_ticks[image_index] = scheduler.currentTick(); };

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