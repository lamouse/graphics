#pragma once
#include <boost/container/small_vector.hpp>
#include "common/common_types.hpp"
#include "update_descriptor.hpp"
#include "shader_tools/shader_info.h"
#include <spdlog/spdlog.h>
#include "render_core/render_vulkan/texture_cache.hpp"
#include <unordered_set>
#include <unordered_map>
import render.vulkan.common;
namespace render::vulkan::pipeline {
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
                use_push_descriptor ? vk::DescriptorSetLayoutCreateFlagBits::ePushDescriptor
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
                use_push_descriptor ? vk::DescriptorUpdateTemplateType::ePushDescriptors
                                    : vk::DescriptorUpdateTemplateType::eDescriptorSet;
            ;
            return device->logical().createDescriptorUpdateTemplate(
                vk::DescriptorUpdateTemplateCreateInfo()
                    .setDescriptorUpdateEntries(entries)
                    .setTemplateType(type)
                    .setDescriptorSetLayout(descriptor_set_layout)
                    .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
                    .setPipelineLayout(pipeline_layout)
                    .setSet(0));
        }

        [[nodiscard]] auto CreatePipelineLayout(vk::DescriptorSetLayout descriptor_set_layout,
                                                uint32_t push_constant_size) const
            -> PipelineLayout {
            const vk::PushConstantRange range =
                vk::PushConstantRange()
                    .setStageFlags(is_compute ? vk::ShaderStageFlagBits::eCompute
                                              : vk::ShaderStageFlagBits::eAllGraphics)
                    .setSize(push_constant_size);
            auto layout = vk::PipelineLayoutCreateInfo()
                              .setSetLayoutCount(descriptor_set_layout ? 1u : 0u)
                              .setPSetLayouts(bindings.empty() ? nullptr : &descriptor_set_layout);

            if (push_constant_size > 0) {
                layout.setPushConstantRanges(range);
            }
            return device->logical().createPipelineLayout(layout);
        }
        void Add(const shader::Info& info, vk::ShaderStageFlags stage) {
            is_compute |=
                (stage & vk::ShaderStageFlagBits::eCompute) == vk::ShaderStageFlagBits::eCompute;

            Add(vk::DescriptorType::eUniformBuffer, stage, info.uniform_buffer_descriptors);
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
                int shader_set = descriptors[i].set;
                int shader_binding = descriptors[i].binding;
                const auto& [shader_sets, is_new] = already_binding.try_emplace(shader_set);
                if (is_new) {
                    shader_sets->second = std::unordered_set<int>{shader_binding};
                } else {
                    auto& sets = shader_sets->second;
                    if (sets.contains(shader_binding)) {
                        spdlog::debug("binding:{} add other stage={}, type={} descriptors={} ",
                                      descriptors[i].binding, vk::to_string(stage),
                                      vk::to_string(type), descriptors.size());
                        for (auto& bind : bindings) {
                            if (bind.binding == static_cast<u32>(shader_binding)) {
                                bind.stageFlags |= stage;
                                break;
                            }
                        }
                        continue;
                    }
                    sets.insert(shader_binding);
                }
                spdlog::debug(
                    "Adding descriptor type={} stage={} \ndescriptors={} binding:{} descriptors "
                    "count:{}",
                    vk::to_string(type), vk::to_string(stage), descriptors.size(),
                    descriptors[i].binding, descriptors[i].count);
                bindings.push_back(vk::DescriptorSetLayoutBinding()
                                       .setBinding(descriptors[i].binding)
                                       .setDescriptorType(type)
                                       .setDescriptorCount(descriptors[i].count)
                                       .setStageFlags(stage));
                entries.push_back(vk::DescriptorUpdateTemplateEntry{
                    descriptors[i].binding,
                    0,
                    descriptors[i].count,
                    type,
                    offset,
                    sizeof(DescriptorUpdateEntry),
                });
                num_descriptors += descriptors[i].count;
                offset += sizeof(DescriptorUpdateEntry);
            }
        }

        const Device* device{};
        bool is_compute{};
        boost::container::small_vector<vk::DescriptorSetLayoutBinding, 32> bindings;
        boost::container::small_vector<vk::DescriptorUpdateTemplateEntry, 32> entries;
        u32 num_descriptors{};
        size_t offset{};
        // set (binding)
        std::unordered_map<int, std::unordered_set<int>> already_binding;
};

}  // namespace render::vulkan::pipeline