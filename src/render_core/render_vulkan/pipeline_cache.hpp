#pragma once
#include <array>
#include <filesystem>
#include <memory>
#include "render_core/render_vulkan/buffer_cache.h"
#include "common/common_types.hpp"
#include "common/common_funcs.hpp"
#include "render_core/render_vulkan/graphics_pipeline.hpp"
#include "render_core/render_vulkan/texture_cache.hpp"
#include "render_core/shader_cache.hpp"
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

struct PipelineCacheHeader {
        static constexpr uint32_t MAGIC = 0x43504B56;  // 'VKPC' in little-endian

        uint32_t magic;
        uint32_t version;
        uint64_t hash;  // xxHash64 of your pipeline key (shaders + state)
};

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
namespace scheduler {
class Scheduler;
}
namespace resource {
class DescriptorPool;
}

class PipelineCache : public ShaderCache {
    public:
        PipelineCache(const Device& device, scheduler::Scheduler& scheduler,
                      resource::DescriptorPool& descriptor_pool,
                      GuestDescriptorQueue& guest_descriptor_queue,
                      RenderPassCache& render_pass_cache, BufferCache& buffer_cache,
                      TextureCache& texture_cache, ShaderNotify& shader_notify);
        ~PipelineCache();
        CLASS_NON_COPYABLE(PipelineCache);
        CLASS_NON_MOVEABLE(PipelineCache);
        [[nodiscard]] auto currentGraphicsPipeline(PrimitiveTopology topology) -> GraphicsPipeline*;
        [[nodiscard]] auto currentComputePipeline(const std::array<u32, 3>& workgroupSize)
            -> ComputePipeline*;

    private:
        auto createGraphicsPipeline() -> std::unique_ptr<GraphicsPipeline>;
        [[nodiscard]] auto currentGraphicsPipelineSlowPath() -> GraphicsPipeline*;
        [[nodiscard]] auto builtPipeline(GraphicsPipeline* pipeline) const noexcept
            -> GraphicsPipeline*;

        auto createGraphicsPipeline(const GraphicsPipelineCacheKey& key, bool build_in_parallel)
            -> std::unique_ptr<GraphicsPipeline>;

        auto CreateComputePipeline(const ComputePipelineCacheKey& key)
            -> std::unique_ptr<ComputePipeline>;

        auto CreateComputePipeline(const ComputePipelineCacheKey& key, bool build_in_parallel)
            -> std::unique_ptr<ComputePipeline>;

        [[nodiscard]] auto CurrentGraphicsPipelineSlowPath() -> GraphicsPipeline*;

        void loadPipelineCacheFromDisk();
        void savePipelineCache();

        void updateShaderHash();
        void updatePipelineKeyState(PrimitiveTopology topology);

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

        std::unordered_map<ComputePipelineCacheKey, std::unique_ptr<ComputePipeline>> compute_cache;
        std::unordered_map<GraphicsPipelineCacheKey, std::unique_ptr<GraphicsPipeline>>
            graphics_cache;

        std::filesystem::path pipeline_cache_filename;

        std::filesystem::path vulkan_pipeline_cache_filename;
        VulkanPipelineCache vulkan_pipeline_cache;
        common::ThreadWorker workers;
        common::ThreadWorker serialization_thread;
        DynamicFeatures dynamic_features;
        PipelineCacheHeader pipe_line_cache_header{};
};

}  // namespace render::vulkan