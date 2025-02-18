#pragma once
#include <vulkan/vulkan.hpp>
#include "buffer_cache.h"
#include "common/common_funcs.hpp"
#include "common/common_types.hpp"
#include "render_core/fixed_pipeline_state.h"
#include "render_core/render_vulkan/descriptor_pool.hpp"
#include "shader_tools/shader_info.h"
#include <condition_variable>
#include "render_core/framebufferConfig.hpp"
#include "render_core/vulkan_common/vulkan_wrapper.hpp"
#include "render_core/render_vulkan/update_descriptor.hpp"
#include "common/thread_worker.hpp"
#include "render_core/render_vulkan/texture_cache.hpp"
namespace render {
class ShaderNotify;
}
namespace render::vulkan {
class Device;
class RenderPassCache;
namespace scheduler {
class Scheduler;
}  // namespace scheduler

struct GraphicsPipelineCacheKey {
        std::array<u64, 6> unique_hashes;
        FixedPipelineState state;

        [[nodiscard]] auto Hash() const noexcept -> size_t;

        auto operator==(const GraphicsPipelineCacheKey& rhs) const noexcept -> bool;

        auto operator!=(const GraphicsPipelineCacheKey& rhs) const noexcept -> bool {
            return !operator==(rhs);
        }

        [[nodiscard]] auto Size() const noexcept -> size_t {
            return sizeof(unique_hashes) + state.Size();
        }
};
static_assert(std::has_unique_object_representations_v<GraphicsPipelineCacheKey>);
static_assert(std::is_trivially_copyable_v<GraphicsPipelineCacheKey>);
static_assert(std::is_trivially_constructible_v<GraphicsPipelineCacheKey>);

}  // namespace render::vulkan
namespace std {
template <>
struct hash<render::vulkan::GraphicsPipelineCacheKey> {
        auto operator()(const render::vulkan::GraphicsPipelineCacheKey& k) const noexcept
            -> size_t {
            return k.Hash();
        }
};
}  // namespace std
namespace render::vulkan {
namespace scheduler {
class Scheduler;
}
namespace resource {
class DescriptorAllocator;
class DescriptorPool;
}  // namespace resource
namespace pipeline {
class PipelineStatistics;
class RescalingPushConstant;
class RenderAreaPushConstant;
}  // namespace pipeline
class GraphicsPipeline {
        static constexpr size_t NUM_STAGES = 5;

    public:
        template <typename Spec>
        static auto MakeConfigureSpecFunc() {
            return
                [](GraphicsPipeline* pl, bool is_indexed) { pl->configureImpl<Spec>(is_indexed); };
        }

        explicit GraphicsPipeline(
            scheduler::Scheduler& scheduler, VulkanPipelineCache& pipeline_cache,
            render::ShaderNotify* shader_notify, const Device& device,
            resource::DescriptorPool& descriptor_pool, GuestDescriptorQueue& guest_descriptor_queue,
            common::ThreadWorker* worker_thread, pipeline::PipelineStatistics* pipeline_statistics,
            RenderPassCache& render_pass_cache, const GraphicsPipelineCacheKey& key,
            TextureCache& texture_cache, BufferCache& buffer_cache, std::array<ShaderModule, NUM_STAGES> stages,
            const std::array<const shader::Info*, NUM_STAGES>& infos, DynamicFeatures dynamic);

        CLASS_NON_COPYABLE(GraphicsPipeline);
        CLASS_NON_MOVEABLE(GraphicsPipeline);
        void AddTransition(GraphicsPipeline* transition);
        void Configure(bool is_indexed) { configure_func(this, is_indexed); }

        [[nodiscard]] auto Next(const GraphicsPipelineCacheKey& current_key) noexcept
            -> GraphicsPipeline* {
            if (key_ == current_key) {
                return this;
            }
            const auto it{std::find(transition_keys.begin(), transition_keys.end(), current_key)};
            return it != transition_keys.end()
                       ? transitions[std::distance(transition_keys.begin(), it)]
                       : nullptr;
        }

        [[nodiscard]] auto IsBuilt() const noexcept -> bool {
            return is_built.load(std::memory_order::relaxed);
        }
        ~GraphicsPipeline();

    private:
        template <typename Spec>
        void configureImpl(bool is_indexed);

        void makePipeline(vk::RenderPass render_pass);
        void ConfigureDraw();
        void validate();

        const GraphicsPipelineCacheKey key_;
        const Device& device_;
        VulkanPipelineCache& pipeline_cache;
        scheduler::Scheduler& scheduler_;
        GuestDescriptorQueue& guest_descriptor_queue_;

        void (*configure_func)(GraphicsPipeline*, bool){};

        std::vector<GraphicsPipelineCacheKey> transition_keys;
        std::vector<GraphicsPipeline*> transitions;

        std::array<ShaderModule, NUM_STAGES> spv_modules_;

        std::array<shader::Info, NUM_STAGES> stage_infos;
        std::array<u32, 5> enabled_uniform_buffer_masks{};
        render::frame::UniformBufferSizes uniform_buffer_sizes{};
        u32 num_textures{};
        DynamicFeatures dynamic;
        DescriptorSetLayout descriptor_set_layout;
        resource::DescriptorAllocator descriptor_allocator;
        PipelineLayout pipeline_layout;
        DescriptorUpdateTemplate descriptor_update_template;
        Pipeline pipeline;
        TextureCache& texture_cache;
        BufferCache& buffer_cache;
        std::condition_variable build_condvar;
        std::mutex build_mutex;
        texture::RenderTargets render_targets;
        bool is_set_render_target{false};
        std::atomic_bool is_built{false};
        bool uses_push_descriptor{false};
};

}  // namespace render::vulkan