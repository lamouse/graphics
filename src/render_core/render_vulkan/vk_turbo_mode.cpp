// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#if defined(ANDROID) && defined(ARCHITECTURE_arm64)
#include <adrenotools/driver.h>
#endif

#include "common/polyfill_thread.hpp"
#include "common/literals.hpp"
#include "render_vulkan/render_vulkan.hpp"
#include "vulkan_common/vulkan_wrapper.hpp"

#include "render_vulkan/vk_shader_util.hpp"
#include "vk_turbo_mode.hpp"
#include "render_core/host_shaders/vulkan_turbo_mode_comp_spv.h"
namespace render::vulkan {

using namespace common::literals;

TurboMode::TurboMode(const Instance& instance)
#ifndef ANDROID
    : m_device{createDevice(instance, VK_NULL_HANDLE)},
      m_allocator{m_device}
#endif
{
    {
        std::scoped_lock lk{m_submission_lock};
        m_submission_time = std::chrono::steady_clock::now();
    }
    m_thread = std::jthread([&](auto stop_token) { Run(stop_token); });
}

TurboMode::~TurboMode() = default;

void TurboMode::QueueSubmitted() {
    std::scoped_lock lk{m_submission_lock};
    m_submission_time = std::chrono::steady_clock::now();
    m_submission_cv.notify_one();
}

void TurboMode::Run(std::stop_token stop_token) {
#ifndef ANDROID
    auto& dld = m_device.getLogical();

    // Allocate buffer. 2MiB should be sufficient.
    const VkBufferCreateInfo buffer_ci = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = 2_MiB,
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
    };
    Buffer buffer = m_allocator.createBuffer(buffer_ci, MemoryUsage::DeviceLocal);

    // Create the descriptor pool to contain our descriptor.
    static constexpr VkDescriptorPoolSize pool_size{
        .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .descriptorCount = 1,
    };

    auto descriptor_pool =
        VulkanDescriptorPool{dld.createDescriptorPool(VkDescriptorPoolCreateInfo{
                                 .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                                 .pNext = nullptr,
                                 .flags = 0,
                                 .maxSets = 1,
                                 .poolSizeCount = 1,
                                 .pPoolSizes = &pool_size,
                             }),
                             dld};

    // Create the descriptor set layout from the pool.
    static constexpr VkDescriptorSetLayoutBinding layout_binding{
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        .pImmutableSamplers = nullptr,
    };

    auto descriptor_set_layout =
        DescriptorSetLayout{dld.createDescriptorSetLayout(VkDescriptorSetLayoutCreateInfo{
                                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                                .pNext = nullptr,
                                .flags = 0,
                                .bindingCount = 1,
                                .pBindings = &layout_binding,
                            }),
                            dld};

    // Actually create the descriptor set.
    auto descriptor_set = descriptor_pool.Allocate(
        vk::DescriptorSetAllocateInfo{*descriptor_pool, 1, descriptor_set_layout.address()});

    // Create the shader.
    auto shader = utils::buildShader(m_device.getLogical(), VULKAN_TURBO_MODE_COMP_SPV);

    // Create the pipeline layout.
    auto pipeline_layout = PipelineLayout{dld.createPipelineLayout(vk::PipelineLayoutCreateInfo{
                                              {},
                                              1,
                                              descriptor_set_layout.address(),
                                              0,
                                              nullptr,
                                          }),
                                          dld};

    // Actually create the pipeline.
    const vk::PipelineShaderStageCreateInfo shader_stage{
        {}, vk::ShaderStageFlagBits::eCompute, *shader, "main"};

    auto pipeline = Pipeline{dld.createComputePipeline({},
                                                       vk::ComputePipelineCreateInfo{

                                                           {},
                                                           shader_stage,
                                                           *pipeline_layout,
                                                           VK_NULL_HANDLE,
                                                           0,
                                                       })
                                 .value,
                             dld};

    // Create a fence to wait on.
    auto fence = Fence{dld.createFence(VkFenceCreateInfo{
                           .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                           .pNext = nullptr,
                           .flags = 0,
                       }),
                       dld};

    // Create a command pool to allocate a command buffer from.
    auto command_pool = CommandPool{dld.createCommandPool(VkCommandPoolCreateInfo{
                                        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                                        .pNext = nullptr,
                                        .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT |
                                                 VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
                                        .queueFamilyIndex = m_device.getGraphicsFamily(),
                                    }),
                                    dld};

    // Create a single command buffer.
    auto cmdbufs = command_pool.Allocate(1);
    auto cmdbuf = cmdbufs[0];
#endif

    while (!stop_token.stop_requested()) {
#ifdef ANDROID
#ifdef ARCHITECTURE_arm64
        adrenotools_set_turbo(true);
#endif
#else
        // Reset the fence.
        fence.Reset();

        // Update descriptor set.
        const vk::DescriptorBufferInfo buffer_info{
            *buffer,
            0,
            VK_WHOLE_SIZE,
        };

        const vk::WriteDescriptorSet buffer_write{
            descriptor_set[0], 0, 0, vk::DescriptorType::eUniformBuffer, {}, buffer_info};

        dld.updateDescriptorSets(std::array{buffer_write}, {});

        // Set up the command buffer.
        cmdbuf.begin(VkCommandBufferBeginInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext = nullptr,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
            .pInheritanceInfo = nullptr,
        });

        // Clear the buffer.
        cmdbuf.fillBuffer(*buffer, 0, VK_WHOLE_SIZE, 0);

        // Bind descriptor set.
        cmdbuf.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *pipeline_layout, 0,
                                  descriptor_set, {});

        // Bind the pipeline.
        cmdbuf.bindPipeline(vk::PipelineBindPoint::eCompute, *pipeline);

        // Dispatch.
        cmdbuf.dispatch(64, 64, 1);

        // Finish.
        cmdbuf.end();

        const vk::SubmitInfo submit_info{
            {},
            {},
            cmdbuf,
        };

        m_device.getGraphicsQueue().submit(std::array{submit_info}, *fence);

        // Wait for completion.
        fence.Wait();
#endif
        // Wait for the next graphics queue submission if necessary.
        std::unique_lock lk{m_submission_lock};
        common::thread::condvarWait(m_submission_cv, lk, stop_token, [this] {
            return (std::chrono::steady_clock::now() - m_submission_time) <=
                   std::chrono::milliseconds{100};
        });
    }
#if defined(ANDROID) && defined(ARCHITECTURE_arm64)
    adrenotools_set_turbo(false);
#endif
}

}  // namespace render::vulkan
