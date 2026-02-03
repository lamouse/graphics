// SPDX-FileCopyrightText: Copyright 2019 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>

#include "common/thread_worker.hpp"
#include "shader_tools/shader_info.h"
#include "render_core/vulkan_common/vulkan_wrapper.hpp"
#include "render_core/render_vulkan/descriptor_pool.hpp"
#include "render_core/buffer_cache/buffer_cache_base.hpp"
#include "render_core/render_vulkan/update_descriptor.hpp"
#include "render_core/render_vulkan/buffer_cache.h"
#include "render_core/render_vulkan/texture_cache.hpp"
#include "render_core/shader_notify.hpp"

namespace render::vulkan {

class Device;
class PipelineStatistics;
namespace scheduler {
class Scheduler;

}

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
        auto operator=(ComputePipeline&&) noexcept -> ComputePipeline& = delete;

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
