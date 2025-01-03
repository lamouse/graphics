#pragma once
#include <vulkan/vulkan.hpp>
#include "common/common_funcs.hpp"
#include "common/common_types.hpp"
#include "shader_tools/shader_info.h"
#include <condition_variable>
#include "framebufferConfig.hpp"
#include "vulkan_common/vulkan_wrapper.hpp"
#include "update_descriptor.hpp"
#include "common/thread_worker.hpp"
namespace render {
class ShaderNotify;
}
namespace render::vulkan {
class Device;
class RenderPassCache;
namespace scheduler {
class Scheduler;
}  // namespace scheduler
namespace pipeline {
struct GraphicsPipelineCacheKey {
        std::array<u64, 6> unique_hashes;

        size_t Hash() const noexcept;

        bool operator==(const GraphicsPipelineCacheKey& rhs) const noexcept;

        bool operator!=(const GraphicsPipelineCacheKey& rhs) const noexcept {
            return !operator==(rhs);
        }

        size_t Size() const noexcept { return sizeof(unique_hashes); }
};
static_assert(std::has_unique_object_representations_v<GraphicsPipelineCacheKey>);
static_assert(std::is_trivially_copyable_v<GraphicsPipelineCacheKey>);
static_assert(std::is_trivially_constructible_v<GraphicsPipelineCacheKey>);
}  // namespace pipeline
}  // namespace render::vulkan
namespace std {
template <>
struct hash<render::vulkan::pipeline::GraphicsPipelineCacheKey> {
        auto operator()(const render::vulkan::pipeline::GraphicsPipelineCacheKey& k) const noexcept
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
            scheduler::Scheduler& scheduler, vk::PipelineCache& pipeline_cache,
            render::ShaderNotify* shader_notify, const Device& device,
            resource::DescriptorPool& descriptor_pool, GuestDescriptorQueue& guest_descriptor_queue,
            common::ThreadWorker* worker_thread, pipeline::PipelineStatistics* pipeline_statistics,
            RenderPassCache& render_pass_cache, const pipeline::GraphicsPipelineCacheKey& key,
            std::array<vk::ShaderModule, NUM_STAGES> stages,
            const std::array<const shader::Info*, NUM_STAGES>& infos);

        CLASS_NON_COPYABLE(GraphicsPipeline);
        CLASS_NON_MOVEABLE(GraphicsPipeline);
        void AddTransition(GraphicsPipeline* transition);
        void Configure(bool is_indexed) { configure_func(this, is_indexed); }

        [[nodiscard]] auto Next(const pipeline::GraphicsPipelineCacheKey& current_key) noexcept
            -> GraphicsPipeline* {
            if (key_ == current_key) {
                return this;
            }
            const auto it{std::find(transition_keys.begin(), transition_keys.end(), current_key)};
            return it != transition_keys.end()
                       ? transitions[std::distance(transition_keys.begin(), it)]
                       : nullptr;
        }

        [[nodiscard]] bool IsBuilt() const noexcept {
            return is_built.load(std::memory_order::relaxed);
        }
        ~GraphicsPipeline();

    private:
        template <typename Spec>
        void configureImpl(bool is_indexed);

        // void ConfigureDraw(const RescalingPushConstant& rescaling,
        //                 const RenderAreaPushConstant& render_are);

        void makePipeline(vk::RenderPass render_pass);
        void ConfigureDraw(const pipeline::RescalingPushConstant& rescaling,
                           const pipeline::RenderAreaPushConstant& render_are);
        void validate();

        const pipeline::GraphicsPipelineCacheKey key_;
        const Device& device_;
        vk::PipelineCache& pipeline_cache;
        scheduler::Scheduler& scheduler_;
        GuestDescriptorQueue& guest_descriptor_queue_;

        void (*configure_func)(GraphicsPipeline*, bool){};

        std::vector<pipeline::GraphicsPipelineCacheKey> transition_keys;
        std::vector<GraphicsPipeline*> transitions;

        std::array<ShaderModule, NUM_STAGES> spv_modules_;

        std::array<shader::Info, NUM_STAGES> stage_infos;
        std::array<u32, 5> enabled_uniform_buffer_masks{};
        render::frame::UniformBufferSizes uniform_buffer_sizes{};
        u32 num_textures{};

        DescriptorSetLayout descriptor_set_layout;
        // resource::DescriptorAllocator descriptor_allocator;
        PipelineLayout pipeline_layout;
        DescriptorUpdateTemplate descriptor_update_template;
        Pipeline pipeline;

        std::condition_variable build_condvar;
        std::mutex build_mutex;
        std::atomic_bool is_built{false};
        bool uses_push_descriptor{false};
};

}  // namespace render::vulkan