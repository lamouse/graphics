#pragma once
#include <boost/container/small_vector.hpp>
#include "common/common_types.hpp"
#include "vulkan_common/vulkan_wrapper.hpp"
#include "vulkan_common/device.hpp"
#include "update_descriptor.hpp"
#include "shader_tools/shader_info.h"
namespace render::vulkan::pipeline {
constexpr u32 NUM_TEXTURE_SCALING_WORDS = 4;
constexpr u32 NUM_IMAGE_SCALING_WORDS = 2;
constexpr u32 NUM_TEXTURE_AND_IMAGE_SCALING_WORDS =
    NUM_TEXTURE_SCALING_WORDS + NUM_IMAGE_SCALING_WORDS;
struct RescalingLayout {
        alignas(16) std::array<u32, NUM_TEXTURE_SCALING_WORDS> rescaling_textures;
        alignas(16) std::array<u32, NUM_IMAGE_SCALING_WORDS> rescaling_images;
        u32 down_factor;
};
struct RenderAreaLayout {
        std::array<f32, 4> render_area;
};
class DescriptorLayoutBuilder {
    public:
        DescriptorLayoutBuilder(const Device& device_) : device{&device_} {}

        [[nodiscard]] auto CanUsePushDescriptor() const noexcept -> bool {
            return device->isKhrPushDescriptorSupported() &&
                   num_descriptors <= device->maxPushDescriptors();
        }

        [[nodiscard]] auto CreateDescriptorSetLayout(bool use_push_descriptor) const
            -> DescriptorSetLayout {
            if (bindings.empty()) {
                return nullptr;
            }

            const vk::DescriptorSetLayoutCreateFlags flags =
                use_push_descriptor ? vk::DescriptorSetLayoutCreateFlagBits::ePushDescriptorKHR
                                    : vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool;
            return DescriptorSetLayout{device->getLogical().createDescriptorSetLayout(
                                           vk::DescriptorSetLayoutCreateInfo{flags, bindings}),
                                       device->getLogical()};
        }

        [[nodiscard]] auto CreateTemplate(vk::DescriptorSetLayout descriptor_set_layout,
                                          vk::PipelineLayout pipeline_layout,
                                          bool use_push_descriptor) const
            -> DescriptorUpdateTemplate {
            if (entries.empty()) {
                return nullptr;
            }
            const vk::DescriptorUpdateTemplateType type =
                use_push_descriptor ? vk::DescriptorUpdateTemplateType::ePushDescriptorsKHR
                                    : vk::DescriptorUpdateTemplateType::eDescriptorSet;
            return device->logical().createDescriptorUpdateTemplate(
                vk::DescriptorUpdateTemplateCreateInfo{
                    {},
                    entries,
                    type,
                    descriptor_set_layout,
                    vk::PipelineBindPoint::eGraphics,
                    pipeline_layout,
                    0,
                });
        }

        [[nodiscard]] auto CreatePipelineLayout(vk::DescriptorSetLayout descriptor_set_layout) const
            -> PipelineLayout {
            const u32 size_offset = is_compute ? sizeof(RescalingLayout::down_factor) : 0u;
            const vk::PushConstantRange range{
                static_cast<vk::ShaderStageFlags>(is_compute
                                                      ? vk::ShaderStageFlagBits::eCompute
                                                      : vk::ShaderStageFlagBits::eAllGraphics),
                0,
                static_cast<u32>(sizeof(RescalingLayout)) - size_offset +
                    static_cast<u32>(sizeof(RenderAreaLayout)),
            };

            return device->logical().createPipelineLayout(vk::PipelineLayoutCreateInfo{
                {},
                descriptor_set_layout ? 1u : 0u,
                bindings.empty() ? nullptr : &descriptor_set_layout,
                1,
                &range,
            });
        }
        void Add(const shader::Info& info, vk::ShaderStageFlags stage) {
            is_compute |=
                (stage & vk::ShaderStageFlagBits::eCompute) == vk::ShaderStageFlagBits::eCompute;

            Add(vk::DescriptorType::eUniformBuffer, stage, info.constant_buffer_descriptors);
            Add(vk::DescriptorType::eStorageBuffer, stage, info.storage_buffers_descriptors);
            Add(vk::DescriptorType::eUniformTexelBuffer, stage, info.texture_buffer_descriptors);
            Add(vk::DescriptorType::eStorageTexelBuffer, stage, info.image_buffer_descriptors);
            Add(vk::DescriptorType::eCombinedImageSampler, stage, info.texture_descriptors);
            Add(vk::DescriptorType::eStorageImage, stage, info.image_descriptors);
        }

    private:
        template <typename Descriptors>
        void Add(vk::DescriptorType type, vk::ShaderStageFlags stage,
                 const Descriptors& descriptors) {
            const size_t num{descriptors.size()};
            for (size_t i = 0; i < num; ++i) {
                bindings.push_back(vk::DescriptorSetLayoutBinding{
                    binding,
                    type,
                    descriptors[i].count,
                    stage,
                    nullptr,
                });
                entries.push_back(vk::DescriptorUpdateTemplateEntry{
                    binding,
                    0,
                    descriptors[i].count,
                    type,
                    offset,
                    sizeof(DescriptorUpdateEntry),
                });
                ++binding;
                num_descriptors += descriptors[i].count;
                offset += sizeof(DescriptorUpdateEntry);
            }
        }

        const Device* device{};
        bool is_compute{};
        boost::container::small_vector<vk::DescriptorSetLayoutBinding, 32> bindings;
        boost::container::small_vector<vk::DescriptorUpdateTemplateEntry, 32> entries;
        u32 binding{};
        u32 num_descriptors{};
        size_t offset{};
};

}  // namespace render::vulkan::pipeline