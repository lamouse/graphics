// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <vector>

#include <boost/container/small_vector.hpp>

#include "render_core/render_vulkan/compute_pipeline.hpp"
#include "render_core/vulkan_common/device.hpp"
#include "render_core/render_vulkan/scheduler.hpp"
#include "render_core/render_vulkan/pipeline_statistics.hpp"
#include "render_core/render_vulkan/descriptor_pool.hpp"
#include "render_core/render_vulkan/pipeline_helper.hpp"

namespace render::vulkan {

ComputePipeline::ComputePipeline(const Device& device_, VulkanPipelineCache& pipeline_cache_,
                                 resource::DescriptorPool& descriptor_pool,
                                 GuestDescriptorQueue& guest_descriptor_queue_,
                                 common::ThreadWorker* thread_worker,
                                 pipeline::PipelineStatistics* pipeline_statistics,
                                 ShaderNotify* shader_notify, const shader::Info& info_,
                                 ShaderModule spv_module_)
    : device{device_},
      pipeline_cache(pipeline_cache_),
      guest_descriptor_queue(guest_descriptor_queue_),
      info{info_},
      spv_module(std::move(spv_module_)) {
    if (shader_notify) {
        shader_notify->MarkShaderBuilding();
    }

    auto func{[this, &descriptor_pool, shader_notify, pipeline_statistics] {
        pipeline::DescriptorLayoutBuilder builder{device};
        builder.Add(info, vk::ShaderStageFlagBits::eCompute);

        descriptor_set_layout = builder.CreateDescriptorSetLayout(false);
        pipeline_layout = builder.CreatePipelineLayout(*descriptor_set_layout);
        descriptor_update_template =
            builder.CreateTemplate(*descriptor_set_layout, *pipeline_layout, false);
        descriptor_allocator = descriptor_pool.allocator(*descriptor_set_layout, info);
        const vk::PipelineShaderStageRequiredSubgroupSizeCreateInfoEXT subgroup_size_ci{
            GUEST_WARP_SIZE,
        };
        vk::PipelineCreateFlags flags{};
        if (device.IsKhrPipelineExecutablePropertiesEnabled()) {
            flags |= vk::PipelineCreateFlagBits::eCaptureStatisticsKHR;
        }
        pipeline = device.logical().createPipeline(
            vk::ComputePipelineCreateInfo()
                .setFlags(flags)
                .setStage(vk::PipelineShaderStageCreateInfo()
                              .setStage(vk::ShaderStageFlagBits::eCompute)
                              .setModule(*spv_module)
                              .setPName("main")
                              .setPNext(device.isExtSubgroupSizeControlSupported()
                                            ? &subgroup_size_ci
                                            : nullptr))
                .setLayout(*pipeline_layout),
            *pipeline_cache);

        if (pipeline_statistics) {
            pipeline_statistics->Collect(*pipeline);
        }
        std::scoped_lock lock{build_mutex};
        is_built = true;
        build_condvar.notify_one();
        if (shader_notify) {
            shader_notify->MarkShaderComplete();
        }
    }};
    if (thread_worker) {
        thread_worker->QueueWork(std::move(func));
    } else {
        func();
    }
}

void ComputePipeline::Configure(scheduler::Scheduler& scheduler, BufferCache& buffer_cache) {
    guest_descriptor_queue.Acquire();

    for (const auto& desc : info.storage_buffers_descriptors) {
        ASSERT(desc.count == 1);
    }

    buffer_cache.DoUpdateComputeBuffers();
    const void* const descriptor_data{guest_descriptor_queue.UpdateData()};

    scheduler.record([this, descriptor_data](vk::CommandBuffer cmdbuf) {
        cmdbuf.bindPipeline(vk::PipelineBindPoint::eCompute, *pipeline);
        if (!descriptor_set_layout) {
            return;
        }
        const vk::DescriptorSet descriptor_set{descriptor_allocator.commit()};
        const vk::Device& dev{device.getLogical()};
        dev.updateDescriptorSetWithTemplate(descriptor_set, *descriptor_update_template,
                                            descriptor_data);
        cmdbuf.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *pipeline_layout, 0,
                                  descriptor_set, nullptr);
    });
}

}  // namespace render::vulkan
