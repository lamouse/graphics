module;
#include <algorithm>
#include "common/common_funcs.hpp"
#include "common/common_types.hpp"
#include "render_core/pipeline_state.h"
#include "shader_tools/shader_info.h"
#include "render_core/shader_notify.hpp"
#include <condition_variable>
#include "common/thread_worker.hpp"
#include <vulkan/vulkan.hpp>
export module render.vulkan:graphics_pipeline;
import render.vulkan.common;
import :texture_cache;
import render.vulkan.render_pass;
import :buffer_cache;
import :descriptor_pool;

export namespace render::vulkan {
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
export namespace render::vulkan {
namespace scheduler {
class Scheduler;
}
namespace pipeline {
class PipelineStatistics;
class RenderAreaPushConstant;
}  // namespace pipeline

class GraphicsPipeline {
        static constexpr size_t NUM_STAGES = 5;

    public:
        explicit GraphicsPipeline(
            scheduler::Scheduler& scheduler, VulkanPipelineCache& pipeline_cache,
            render::ShaderNotify* shader_notify, const Device& device,
            resource::DescriptorPool& descriptor_pool, GuestDescriptorQueue& guest_descriptor_queue,
            common::ThreadWorker* worker_thread, RenderPassCache& render_pass_cache,
            const GraphicsPipelineCacheKey& key, TextureCache& texture_cache,
            BufferCache& buffer_cache, std::array<ShaderModule, NUM_STAGES> stages,
            const std::array<const shader::Info*, NUM_STAGES>& infos, DynamicFeatures dynamic);

        CLASS_NON_COPYABLE(GraphicsPipeline);
        CLASS_NON_MOVEABLE(GraphicsPipeline);
        void AddTransition(GraphicsPipeline* transition);
        void Configure();
        auto HasDynamicVertexInput() const noexcept -> bool {
            return key_.state.dynamic_vertex_input;
        }
        [[nodiscard]] auto Next(const GraphicsPipelineCacheKey& current_key) noexcept
            -> GraphicsPipeline* {
            if (key_ == current_key) {
                return this;
            }
            const auto it{std::ranges::find(transition_keys, current_key)};
            return it != transition_keys.end()
                       ? transitions[std::distance(transition_keys.begin(), it)]
                       : nullptr;
        }

        [[nodiscard]] auto IsBuilt() const noexcept -> bool {
            return is_built.load(std::memory_order::relaxed);
        }
        ~GraphicsPipeline();

    private:
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
        std::atomic_bool is_built{false};
        bool uses_push_descriptor{false};

        bool use_dynamic_render;
};

}  // namespace render::vulkan