#include "pipeline_cache.hpp"
#include "descriptor_pool.hpp"
#include "render_vulkan/pipeline_statistics.hpp"
#include "scheduler.hpp"
#include "render_pass.hpp"
#include <filesystem>
#include <fstream>
#include <memory>
#include <utility>
#include "render_core/vulkan_common/device.hpp"
#include "common/settings.hpp"
#include "render_core/render_vulkan/compute_pipeline.hpp"
#include "render_core/render_vulkan/vk_shader_util.hpp"
#include "shader_tools/shader_compile.hpp"
#include <farmhash.h>
namespace render::vulkan {

namespace {

constexpr u32 CACHE_VERSION = 11;

auto GetTotalPipelineWorkers() -> size_t {
    const size_t max_core_threads =
        std::max<size_t>(static_cast<size_t>(std::thread::hardware_concurrency()), 2ULL) - 1ULL;
#ifdef ANDROID
    // Leave at least a few cores free in android
    constexpr size_t free_cores = 3ULL;
    if (max_core_threads <= free_cores) {
        return 1ULL;
    }
    return max_core_threads - free_cores;
#else
    return max_core_threads;
#endif
}
}  // namespace

auto ComputePipelineCacheKey::Hash() const noexcept -> size_t {
    const u64 hash = NAMESPACE_FOR_HASH_FUNCTIONS::Fingerprint64(
        reinterpret_cast<const char*>(this), sizeof *this);
    return static_cast<size_t>(hash);
}

auto ComputePipelineCacheKey::operator==(const ComputePipelineCacheKey& rhs) const noexcept
    -> bool {
    return std::memcmp(&rhs, this, sizeof *this) == 0;
}

PipelineCache::PipelineCache(const Device& device, scheduler::Scheduler& scheduler,
                             resource::DescriptorPool& descriptor_pool,
                             GuestDescriptorQueue& guest_descriptor_queue_,
                             RenderPassCache& render_pass_cache, BufferCache& buffer_cache,
                             TextureCache& texture_cache, ShaderNotify& shader_notify_)
    : device(device),
      scheduler(scheduler),
      descriptor_pool(descriptor_pool),
      guest_descriptor_queue(guest_descriptor_queue_),
      render_pass_cache(render_pass_cache),
      buffer_cache(buffer_cache),
      texture_cache(texture_cache),
      shader_notify(shader_notify_),
      use_asynchronous_shaders(
          common::settings::get<settings::Graphics>().use_asynchronous_shaders),
      use_vulkan_pipeline_cache(common::settings::get<settings::RenderVulkan>().use_pipeline_cache),
      workers(device.hasBrokenParallelShaderCompiling() ? 1ULL : GetTotalPipelineWorkers(),
              "VkPipelineBuilder"),
      serialization_thread(1, "vkPipelineSerialization") {
    dynamic_features = DynamicFeatures{
        .has_extended_dynamic_state = device.IsExtExtendedDynamicStateSupported(),
        .has_extended_dynamic_state_2 = device.IsExtExtendedDynamicState2Supported(),
        .has_extended_dynamic_state_2_extra = device.IsExtExtendedDynamicState2ExtrasSupported(),
        .has_extended_dynamic_state_3_blend = device.IsExtExtendedDynamicState3BlendingSupported(),
        .has_extended_dynamic_state_3_enables = device.IsExtExtendedDynamicState3EnablesSupported(),
        .has_dynamic_vertex_input = device.IsExtVertexInputDynamicStateSupported(),
    };
}

PipelineCache::~PipelineCache() {

}

auto PipelineCache::currentGraphicsPipeline(PrimitiveTopology topology) -> GraphicsPipeline* {
    graphics_key.unique_hashes.at(static_cast<u8>(ShaderType::Vertex)) =
        shader_infos.at(static_cast<u8>(ShaderType::Vertex))->unique_hash;
    graphics_key.unique_hashes.at(static_cast<u8>(ShaderType::Fragment)) =
        shader_infos.at(static_cast<u8>(ShaderType::Fragment))->unique_hash;
        graphics_key.state.topology = topology;
    if (current_pipeline) {
        GraphicsPipeline* const next{current_pipeline->Next(graphics_key)};
        if (next) {
            current_pipeline = next;
            return builtPipeline(current_pipeline);
        }
    }
    return CurrentGraphicsPipelineSlowPath();
}

[[nodiscard]] auto PipelineCache::CurrentGraphicsPipelineSlowPath() -> GraphicsPipeline* {
    const auto [pair, is_new]{graphics_cache.try_emplace(graphics_key)};
    auto& pipeline{pair->second};
    if (is_new) {
        pipeline = createGraphicsPipeline();
    }
    if (!pipeline) {
        return nullptr;
    }
    if (current_pipeline) {
        current_pipeline->AddTransition(pipeline.get());
    }
    current_pipeline = pipeline.get();
    return builtPipeline(current_pipeline);
}

auto PipelineCache::currentComputePipeline(const std::array<u32, 3>& workgroupSize) -> ComputePipeline* {
    const ShaderInfo* shader = shader_infos.at(static_cast<u8>(ShaderType::Compute));
    if (!shader) {
        return nullptr;
    }
    const ComputePipelineCacheKey key{
        .unique_hash = shader->unique_hash,
        .shared_memory_size = 0,
        .workgroup_size = workgroupSize,
    };
    const auto [pair, is_new]{compute_cache.try_emplace(key)};
    auto& pipeline{pair->second};
    if (!is_new) {
        return pipeline.get();
    }
    pipeline = CreateComputePipeline(key);
    mark_loaded(shader_infos);
    return pipeline.get();
}

auto PipelineCache::currentGraphicsPipelineSlowPath() -> GraphicsPipeline* {
    const auto [pair, is_new]{graphics_cache.try_emplace(graphics_key)};
    auto& pipeline{pair->second};
    if (is_new) {
        pipeline = createGraphicsPipeline();
    }
    if (!pipeline) {
        return nullptr;
    }
    if (current_pipeline) {
        current_pipeline->AddTransition(pipeline.get());
    }
    current_pipeline = pipeline.get();
    return builtPipeline(current_pipeline);
}

auto PipelineCache::builtPipeline(GraphicsPipeline* pipeline) const noexcept -> GraphicsPipeline* {
    if (pipeline->IsBuilt()) {
        return pipeline;
    }
    if (!use_asynchronous_shaders) {
        return pipeline;
    }

    return nullptr;
}

auto PipelineCache::createGraphicsPipeline(const GraphicsPipelineCacheKey& key,
                                           pipeline::PipelineStatistics* statistics,
                                           bool build_in_parallel)
    -> std::unique_ptr<GraphicsPipeline> try {
    SPDLOG_INFO("create pipeline 0x{:016x}", key.Hash());
    std::array<ShaderModule, 5> modules;

    modules[0] = utils::buildShader(
        device.getLogical(),
        getShaderData(shader_infos.at(static_cast<u8>(ShaderType::Vertex))->unique_hash));
    modules[4] = utils::buildShader(
        device.getLogical(),
        getShaderData(shader_infos.at(static_cast<u8>(ShaderType::Fragment))->unique_hash));
    std::array<const shader::Info*, 5> infos{};
    shader::Info frag_info = shader::compile::getShaderInfo(
        getShaderData(shader_infos.at(static_cast<u8>(ShaderType::Fragment))->unique_hash));
    shader::Info vertex_info = shader::compile::getShaderInfo(
        getShaderData(shader_infos.at(static_cast<u8>(ShaderType::Vertex))->unique_hash));

    infos[0] = &vertex_info;
    infos[4] = &frag_info;
    common::ThreadWorker* const thread_worker{build_in_parallel ? &workers : nullptr};
    auto pipeline = std::make_unique<GraphicsPipeline>(
        scheduler, vulkan_pipeline_cache, &shader_notify, device, descriptor_pool,
        guest_descriptor_queue, thread_worker, statistics, render_pass_cache, key, texture_cache,
        buffer_cache, std::move(modules), infos, dynamic_features);
    current_pipeline = pipeline.get();
    if (pipeline) {
        graphics_cache.emplace(key, std::move(pipeline));
    }

    mark_loaded(shader_infos);
    return pipeline;

} catch (const std::exception& exception) {
    SPDLOG_ERROR("{}", exception.what());
    return nullptr;
}

auto PipelineCache::createGraphicsPipeline() -> std::unique_ptr<GraphicsPipeline> {

    graphics_key.state.color_formats[0] = surface::PixelFormat::B8G8R8A8_UNORM;
    for (size_t i = 1; i < graphics_key.state.color_formats.size(); i++) {
        graphics_key.state.color_formats.at(i) = surface::PixelFormat::Invalid;
    }
    graphics_key.state.depth_format = surface::PixelFormat::D32_FLOAT;
    graphics_key.state.msaa_mode = MsaaMode::Msaa1x1;
    graphics_key.state.depth_enabled = 1;
    return createGraphicsPipeline(graphics_key, nullptr, false);
}

auto PipelineCache::CreateComputePipeline(const ComputePipelineCacheKey& key)
    -> std::unique_ptr<ComputePipeline> {
    return CreateComputePipeline(key, nullptr, false);
}

auto PipelineCache::CreateComputePipeline(const ComputePipelineCacheKey& key,
                                          pipeline::PipelineStatistics* statistics,
                                          bool build_in_parallel)
    -> std::unique_ptr<ComputePipeline> try {
    auto hash = key.Hash();
    if (device.HasBrokenCompute()) {
        SPDLOG_ERROR("ComputePipeline Skipping 0x{:016x}", hash);
        return nullptr;
    }
    SPDLOG_INFO("ComputePipeline 0x{:016x}", hash);
    ShaderModule spv_module = utils::buildShader(
        device.getLogical(),
        getShaderData(shader_infos.at(static_cast<u8>(ShaderType::Compute))->unique_hash));
    if (device.hasDebuggingToolAttached()) {
        const auto name{fmt::format("Shader {:016x}", key.unique_hash)};
        spv_module.SetObjectNameEXT(name.c_str());
    }
    shader::Info info = shader::compile::getShaderInfo(
        getShaderData(shader_infos.at(static_cast<u8>(ShaderType::Compute))->unique_hash));
    common::ThreadWorker* const thread_worker{build_in_parallel ? &workers : nullptr};
    mark_loaded(shader_infos);
    return std::make_unique<ComputePipeline>(device, vulkan_pipeline_cache, descriptor_pool,
                                             guest_descriptor_queue, thread_worker, statistics,
                                             &shader_notify, info, std::move(spv_module));
} catch (const std::exception& exception) {
    SPDLOG_ERROR("{}", exception.what());
    return nullptr;
}

}  // namespace render::vulkan