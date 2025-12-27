module;
#include "common/class_traits.hpp"

#include <memory>
#include <vulkan/vulkan.hpp>
#include <functional>
export module render.vulkan.present.layer;
import render.vulkan.common;
import render.vulkan.scheduler;
import render.vulkan.present.present_frame;

import render.vulkan.present.FSR;
import render.vulkan.present.AntiAliasPass;
import render.vulkan.present.push_constants;
import render.framebuffer_config;
import core;
import common;

export namespace render::vulkan {

class Layer final {
    public:
        explicit Layer(const Device& device, MemoryAllocator& memory_allocator,
                       scheduler::Scheduler& scheduler, size_t image_count,
                       vk::Extent2D output_size, vk::DescriptorSetLayout layout);
        ~Layer();

        void ConfigureDraw(PresentPushConstants* out_push_constants,
                           vk::DescriptorSet* out_descriptor_set,
                           const std::function<std::optional<FramebufferTextureInfo>(const frame::FramebufferConfig& framebuffer,
                               uint32_t stride)>& accelerateDisplay,
                           vk::Sampler sampler, size_t image_index,
                           const frame::FramebufferConfig& framebuffer,
                           const layout::FrameBufferLayout& layout);
        CLASS_NON_COPYABLE(Layer);
        CLASS_NON_MOVEABLE(Layer);

    private:
        void CreateDescriptorPool();
        void CreateDescriptorSets(vk::DescriptorSetLayout layout);
        void CreateRawImages(const frame::FramebufferConfig& framebuffer);
        void CreateFSR(vk::Extent2D output_size);

        void RefreshResources(const frame::FramebufferConfig& framebuffer);
        void SetAntiAliasPass();
        void ReleaseRawImages();

        void SetMatrixData(PresentPushConstants& data,
                           const layout::FrameBufferLayout& layout) const;
        void SetVertexData(PresentPushConstants& data, const layout::FrameBufferLayout& layout,
                           const common::Rectangle<f32>& crop) const;
        void UpdateDescriptorSet(vk::ImageView image_view, vk::Sampler sampler, size_t image_index);

        const Device& device;
        MemoryAllocator& memory_allocator;
        scheduler::Scheduler& scheduler;
        const size_t image_count;
        VulkanDescriptorPool descriptor_pool;
        DescriptorSets descriptor_sets;

        std::vector<Image> raw_images;
        std::vector<ImageView> raw_image_views;
        u32 raw_width{};
        u32 raw_height{};

        std::unique_ptr<AntiAliasPass> anti_alias;

        std::unique_ptr<FSR> fsr;
        std::vector<u64> resource_ticks;
};

}
