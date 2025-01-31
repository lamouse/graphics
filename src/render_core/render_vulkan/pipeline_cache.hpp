#pragma once
#include <array>
#include <filesystem>
#include <memory>
#include <stop_token>
#include "render_core/render_vulkan/buffer_cache.h"
#include "common/common_types.hpp"
#include "common/common_funcs.hpp"
#include "render_core/render_vulkan/graphics_pipeline.hpp"
#include "shader_tools/profile.h"
#include "shader_tools/host_translate_info.h"
#include "render_core/render_vulkan/texture_cache.hpp"
namespace render::vulkan {

struct ComputePipelineCacheKey {
        u64 unique_hash;
        u32 shared_memory_size;
        std::array<u32, 3> workgroup_size;
        [[nodiscard]] auto Hash() const noexcept -> size_t;
        auto operator==(const ComputePipelineCacheKey& rhs) const noexcept -> bool;

        auto operator!=(const ComputePipelineCacheKey& rhs) const noexcept -> bool {
            return !operator==(rhs);
        }
};
static_assert(std::has_unique_object_representations_v<ComputePipelineCacheKey>);
static_assert(std::is_trivially_copyable_v<ComputePipelineCacheKey>);
static_assert(std::is_trivially_constructible_v<ComputePipelineCacheKey>);

}  // namespace render::vulkan

namespace std {

template <>
struct hash<render::vulkan::ComputePipelineCacheKey> {
        auto operator()(const render::vulkan::ComputePipelineCacheKey& k) const noexcept -> size_t {
            return k.Hash();
        }
};

}  // namespace std

namespace render::vulkan {
class Device;
class ComputePipeline;

class RenderPassCache;

namespace pipeline {
class PipelineStatistics;
}

namespace scheduler {
class Scheduler;
}
namespace resource {
class DescriptorPool;
}

class PipelineCache {
    public:
        PipelineCache(const Device& device, scheduler::Scheduler& scheduler,
                      resource::DescriptorPool& descriptor_pool,
                      GuestDescriptorQueue& guest_descriptor_queue,
                      RenderPassCache& render_pass_cache, BufferCache& buffer_cache,
                      TextureCache& texture_cache, ShaderNotify& shader_notify);
        ~PipelineCache();
        CLASS_NON_COPYABLE(PipelineCache);
        CLASS_NON_MOVEABLE(PipelineCache);
        [[nodiscard]] auto currentGraphicsPipeline() -> GraphicsPipeline*;
        [[nodiscard]] auto currentComputePipeline() -> ComputePipeline*;
        void loadDiskResource(u64 title_id, std::stop_token stop_loading);
        auto createGraphicsPipeline() -> std::unique_ptr<GraphicsPipeline>;

    private:
        [[nodiscard]] auto currentGraphicsPipelineSlowPath() -> GraphicsPipeline*;
        [[nodiscard]] auto builtPipeline(GraphicsPipeline* pipeline) const noexcept
            -> GraphicsPipeline*;

        auto createGraphicsPipeline(const GraphicsPipelineCacheKey& key,
                                    pipeline::PipelineStatistics* statistics,
                                    bool build_in_parallel) -> std::unique_ptr<GraphicsPipeline>;

        auto CreateComputePipeline(const ComputePipelineCacheKey& key)
            -> std::unique_ptr<ComputePipeline>;

        auto CreateComputePipeline(const ComputePipelineCacheKey& key,
                                   pipeline::PipelineStatistics* statistics, bool build_in_parallel)
            -> std::unique_ptr<ComputePipeline>;

        void SerializeVulkanPipelineCache(const std::filesystem::path& filename,
                                          const VulkanPipelineCache& pipeline_cache,
                                          u32 cache_version);

        auto LoadVulkanPipelineCache(const std::filesystem::path& filename,
                                     u32 expected_cache_version) -> VulkanPipelineCache;

        const Device& device;
        scheduler::Scheduler& scheduler;
        resource::DescriptorPool& descriptor_pool;
        GuestDescriptorQueue& guest_descriptor_queue;
        RenderPassCache& render_pass_cache;
        BufferCache& buffer_cache;
        TextureCache& texture_cache;
        ShaderNotify& shader_notify;

        bool use_asynchronous_shaders{};
        bool use_vulkan_pipeline_cache{};
        GraphicsPipelineCacheKey graphics_key{};
        GraphicsPipeline* current_pipeline{};

        Shader::Profile profile;
        Shader::HostTranslateInfo host_info;

        std::unordered_map<ComputePipelineCacheKey, std::unique_ptr<ComputePipeline>> compute_cache;
        std::unordered_map<GraphicsPipelineCacheKey, std::unique_ptr<GraphicsPipeline>>
            graphics_cache;

        std::filesystem::path pipeline_cache_filename;

        std::filesystem::path vulkan_pipeline_cache_filename;
        VulkanPipelineCache vulkan_pipeline_cache;
        common::ThreadWorker workers;
        common::ThreadWorker serialization_thread;
        DynamicFeatures dynamic_features;
};

}  // namespace render::vulkan