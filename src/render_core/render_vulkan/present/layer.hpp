#pragma once

#include "common/math_util.h"
#include "vulkan_common/vulkan_wrapper.hpp"
#include "core/frontend/framebuffer_layout.hpp"
#include "framebuffer_config.hpp"
#include <memory>
#include <functional>
#include "render_core/render_vulkan/present/present_frame.hpp"
namespace render::vulkan {
class Device;
class FSR;
class MemoryAllocator;
struct PresentPushConstants;
class AntiAliasPass;

namespace scheduler {
class Scheduler;
}

class Layer final {
    public:
        explicit Layer(const Device& device, MemoryAllocator& memory_allocator,
                       scheduler::Scheduler& scheduler, size_t image_count,
                       vk::Extent2D output_size, vk::DescriptorSetLayout layout);
        ~Layer();

        void ConfigureDraw(
            PresentPushConstants* out_push_constants, vk::DescriptorSet* out_descriptor_set,
            const std::function<std::optional<present::FramebufferTextureInfo>(
                const frame::FramebufferConfig& framebuffer, uint32_t stride)>& accelerateDisplay,
            vk::Sampler sampler, size_t image_index, const frame::FramebufferConfig& framebuffer,
            const layout::FrameBufferLayout& layout);
        Layer(const Layer&) = delete;
        Layer(Layer&&) noexcept = delete;
        auto operator=(const Layer&) -> Layer& = delete;
        auto operator=(Layer&&) noexcept ->Layer& = delete;

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
}  // namespace render::vulkan