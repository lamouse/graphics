module;
#include <atomic>
#include <condition_variable>
#include <mutex>

export module render.vulkan.compute_pipeline;

import render.vulkan.common;
import render.vulkan.descriptor_pool;
import render.vulkan.scheduler;
import render.vulkan.update_descriptor;
import render.vulkan.buffer_cache;
import render.shader_notify;
import render.buffer.cache;
import render;
import shader;
import common;

export namespace render::vulkan {

class ComputePipeline {
    public:
        explicit ComputePipeline(const Device& device, VulkanPipelineCache& pipeline_cache,
                                 resource::DescriptorPool& descriptor_pool,
                                 GuestDescriptorQueue& guest_descriptor_queue,
                                 common::ThreadWorker* thread_worker, ShaderNotify* shader_notify,
                                 const shader::Info& info, ShaderModule spv_module);
        ComputePipeline(const ComputePipeline&) = delete;
        ComputePipeline(ComputePipeline&&) noexcept = delete;
        auto operator=(const ComputePipeline&) -> ComputePipeline& = delete;
        auto operator=(ComputePipeline&&) noexcept ->ComputePipeline& = delete;
        ~ComputePipeline() = default;

        void Configure(scheduler::Scheduler& scheduler, BufferCache& buffer_cache);

    private:
        const Device& device;
        VulkanPipelineCache& pipeline_cache;
        GuestDescriptorQueue& guest_descriptor_queue;
        shader::Info info;

        ShaderModule spv_module;
        DescriptorSetLayout descriptor_set_layout;
        resource::DescriptorAllocator descriptor_allocator;
        PipelineLayout pipeline_layout;
        DescriptorUpdateTemplate descriptor_update_template;
        Pipeline pipeline;

        std::condition_variable build_condvar;
        std::mutex build_mutex;
        std::atomic_bool is_built{false};
};

}  // namespace render::vulkan