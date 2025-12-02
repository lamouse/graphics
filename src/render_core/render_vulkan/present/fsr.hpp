#pragma once
#include "vulkan_common/vulkan_wrapper.hpp"
#include "vulkan_common/memory_allocator.hpp"
#include "common/math_util.h"
namespace render::vulkan {
class Device;
namespace scheduler {
class Scheduler;
}

class FSR {
    public:
        explicit FSR(const Device& device, MemoryAllocator& memory_allocator, size_t image_count,
                     vk::Extent2D extent);
        auto Draw(scheduler::Scheduler& scheduler, size_t image_index, vk::Image source_image,
                  vk::ImageView source_image_view, vk::Extent2D input_image_extent,
                  const common::Rectangle<f32>& crop_rect) -> vk::ImageView;

    private:
        void CreateImages();
        void CreateSampler();
        void CreateShaders();
        void CreateDescriptorPool();
        void CreateDescriptorSetLayout();
        void CreateDescriptorSets();
        void CreatePipelineLayouts();
        void CreatePipelines();

        void UploadImages(scheduler::Scheduler& scheduler);
        void UpdateDescriptorSets(vk::ImageView image_view, size_t image_index);

        const Device& m_device;
        MemoryAllocator& m_memory_allocator;
        const size_t m_image_count;
        const vk::Extent2D m_extent;
        enum FsrStage { Easu, Rcas, MaxFsrStage };
        VulkanDescriptorPool m_descriptor_pool;
        DescriptorSetLayout m_descriptor_set_layout;
        PipelineLayout m_pipeline_layout;
        ShaderModule m_vert_shader;
        ShaderModule m_easu_shader;
        ShaderModule m_rcas_shader;
        Pipeline m_easu_pipeline;
        Pipeline m_rcas_pipeline;
        Sampler m_sampler;
        static constexpr vk::Format format_ = vk::Format::eR16G16B16A16Sfloat;

        struct Images {
                DescriptorSets descriptor_sets;
                std::array<Image, MaxFsrStage> images;
                std::array<ImageView, MaxFsrStage> image_views;
        };
        std::vector<Images> m_dynamic_images;
        bool m_images_ready{};
};
}  // namespace render::vulkan