module;
#include "common/assert.hpp"
#include <boost/container/small_vector.hpp>
#include "common/thread_worker.hpp"
#include "shader_tools/shader_info.h"
#include <vulkan/vulkan.hpp>

module render.vulkan.compute_pipeline;
import render.vulkan.common;
import render.vulkan.update_descriptor;
import render.vulkan.pipeline_helper;
import render.shader_notify;

namespace render::vulkan {

ComputePipeline::ComputePipeline(const Device& device_, VulkanPipelineCache& pipeline_cache_,
                                 resource::DescriptorPool& descriptor_pool,
                                 GuestDescriptorQueue& guest_descriptor_queue_,
                                 common::ThreadWorker* thread_worker, ShaderNotify* shader_notify,
                                 const shader::Info& info_, ShaderModule spv_module_)
    : device{device_},
      pipeline_cache(pipeline_cache_),
      guest_descriptor_queue(guest_descriptor_queue_),
      info{info_},
      spv_module(std::move(spv_module_)) {
    if (shader_notify) {
        shader_notify->MarkShaderBuilding();
    }
    uint32_t push_const_size{0};
    if (info_.push_constants.size > 0) {
        push_const_size = info_.push_constants.size;
    }

    auto func{[this, &descriptor_pool, shader_notify, push_const_size] {
        pipeline::DescriptorLayoutBuilder builder{device};
        builder.Add(info, vk::ShaderStageFlagBits::eCompute);

        descriptor_set_layout = builder.CreateDescriptorSetLayout(false);
        pipeline_layout = builder.CreatePipelineLayout(*descriptor_set_layout, push_const_size);
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
