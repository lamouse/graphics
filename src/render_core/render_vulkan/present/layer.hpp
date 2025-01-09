#pragma once

#include "common/math_util.h"
#include "vulkan_common/vulkan_wrapper.hpp"
namespace render::vulkan {
class Device;
class FSR;
class MemoryAllocator;
struct PresentPushConstants;
class RasterizerVulkan;
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

        void ConfigureDraw(PresentPushConstants* out_push_constants,
                           VkDescriptorSet* out_descriptor_set, RasterizerVulkan& rasterizer,
                           VkSampler sampler, size_t image_index);

    private:
        void CreateDescriptorPool();
        void CreateDescriptorSets(VkDescriptorSetLayout layout);
        void CreateStagingBuffer();
        void CreateRawImages();
        void CreateFSR(VkExtent2D output_size);

        void RefreshResources();
        void SetAntiAliasPass();
        void ReleaseRawImages();

        [[nodiscard]] auto CalculateBufferSize() const -> u64;
        [[nodiscard]] auto GetRawImageOffset(size_t image_index) const -> u64;

        void SetMatrixData(PresentPushConstants& data) const;
        void SetVertexData(PresentPushConstants& data, const common::Rectangle<f32>& crop) const;
        void UpdateDescriptorSet(VkImageView image_view, VkSampler sampler, size_t image_index);
        void UpdateRawImage(size_t image_index);

    private:
        const Device& device;
        MemoryAllocator& memory_allocator;
        scheduler::Scheduler& scheduler;
        const size_t image_count;
        vk::DescriptorPool descriptor_pool{};
        DescriptorSets descriptor_sets;

        Buffer buffer;
        std::vector<Image> raw_images{};
        std::vector<ImageView> raw_image_views{};
        u32 raw_width{};
        u32 raw_height{};

        std::unique_ptr<AntiAliasPass> anti_alias;

        std::unique_ptr<FSR> fsr{};
        std::vector<u64> resource_ticks{};
};
}  // namespace render::vulkan