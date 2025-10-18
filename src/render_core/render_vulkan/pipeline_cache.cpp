#include "pipeline_cache.hpp"
#include "descriptor_pool.hpp"
#include "render_vulkan/pipeline_statistics.hpp"
#include "scheduler.hpp"
#include "render_pass.hpp"
#include <filesystem>
#include <fstream>
#include <memory>
#include "render_core/vulkan_common/device.hpp"
#include "common/settings.hpp"
#include "render_core/render_vulkan/compute_pipeline.hpp"
#include "render_core/render_vulkan/vk_shader_util.hpp"
#include "shader_tools/shader_compile.hpp"
#include <farmhash.h>
namespace render::vulkan {

namespace {

constexpr u32 CACHE_VERSION = 11;
constexpr std::array<char, 8> VULKAN_CACHE_MAGIC_NUMBER{'e', 'n', 'g', 'e', 'v', 'k', 'c', 'h'};

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
    if (use_vulkan_pipeline_cache && !vulkan_pipeline_cache_filename.empty()) {
        SerializeVulkanPipelineCache(vulkan_pipeline_cache_filename, vulkan_pipeline_cache,
                                     CACHE_VERSION);
    }
}

auto PipelineCache::currentGraphicsPipeline() -> GraphicsPipeline* {
    if (current_pipeline) {
        return current_pipeline;
    }
    createGraphicsPipeline();  // TODO 临时设置一下后续使用

    return nullptr;

    // if (current_pipeline) {
    //     GraphicsPipeline* const next{current_pipeline->Next(graphics_key)};
    //     if (next) {
    //         current_pipeline = next;
    //         return builtPipeline(current_pipeline);
    //     }
    // }
    // return currentGraphicsPipelineSlowPath();
}

auto PipelineCache::currentComputePipeline() -> ComputePipeline* {
    const ComputePipelineCacheKey key{
        .unique_hash = 0,
        .shared_memory_size = 0,
        .workgroup_size{1, 1, 1},
    };
    const auto [pair, is_new]{compute_cache.try_emplace(key)};
    auto& pipeline{pair->second};
    if (!is_new) {
        return pipeline.get();
    }
    pipeline = CreateComputePipeline(key);
    return pipeline.get();
}

void PipelineCache::loadDiskResource(u64 title_id, std::stop_token stop_loading) {
    if (title_id == 0) {
        return;
    }
    const auto* const shader_dir{"./user/"};
    const auto base_dir{shader_dir + fmt::format("{:016x}", title_id)};
    if (std::filesystem::create_directories(base_dir)) {
        SPDLOG_ERROR("Failed to create pipeline cache directories");
        return;
    }
    pipeline_cache_filename = base_dir + "/vulkan.bin";

    if (use_vulkan_pipeline_cache) {
        vulkan_pipeline_cache_filename = base_dir + "/vulkan_pipelines.bin";
        vulkan_pipeline_cache =
            LoadVulkanPipelineCache(vulkan_pipeline_cache_filename, CACHE_VERSION);
    }

    struct {
            std::mutex mutex;
            size_t total{};
            size_t built{};
            bool has_loaded{};
            std::unique_ptr<pipeline::PipelineStatistics> statistics;
    } state;

    if (device.IsKhrPipelineExecutablePropertiesEnabled()) {
        state.statistics = std::make_unique<pipeline::PipelineStatistics>(device);
    }
    const auto load_compute{[&](std::ifstream& file) {
        ComputePipelineCacheKey key;
        file.read(reinterpret_cast<char*>(&key), sizeof(key));

        workers.QueueWork([this, key, &state]() mutable {
            auto pipeline{CreateComputePipeline(key, state.statistics.get(), false)};
            std::scoped_lock lock{state.mutex};
            if (pipeline) {
                compute_cache.emplace(key, std::move(pipeline));
            }
            ++state.built;
        });
        ++state.total;
    }};
    const auto load_graphics{[&](std::ifstream& file) {
        GraphicsPipelineCacheKey key;
        file.read(reinterpret_cast<char*>(&key), sizeof(key));

        // if ((key.state.extended_dynamic_state != 0) !=
        //         dynamic_features.has_extended_dynamic_state ||
        //     (key.state.extended_dynamic_state_2 != 0) !=
        //         dynamic_features.has_extended_dynamic_state_2 ||
        //     (key.state.extended_dynamic_state_2_extra != 0) !=
        //         dynamic_features.has_extended_dynamic_state_2_extra ||
        //     (key.state.extended_dynamic_state_3_blend != 0) !=
        //         dynamic_features.has_extended_dynamic_state_3_blend ||
        //     (key.state.extended_dynamic_state_3_enables != 0) !=
        //         dynamic_features.has_extended_dynamic_state_3_enables ||
        //     (key.state.dynamic_vertex_input != 0) != dynamic_features.has_dynamic_vertex_input) {
        //     return;
        // }
        workers.QueueWork([this, key, &state]() mutable {
            auto pipeline{createGraphicsPipeline(key, state.statistics.get(), false)};

            std::scoped_lock lock{state.mutex};
            if (pipeline) {
                graphics_cache.emplace(key, std::move(pipeline));
            }
            ++state.built;
            if (state.has_loaded) {
                // callback(VideoCore::LoadCallbackStage::Build, state.built, state.total);
            }
        });
        ++state.total;
    }};

    SPDLOG_INFO("Total Pipeline Count: {}", state.total);

    std::unique_lock lock{state.mutex};
    // callback(VideoCore::LoadCallbackStage::Build, 0, state.total);
    state.has_loaded = true;
    lock.unlock();

    workers.WaitForRequests(stop_loading);

    if (use_vulkan_pipeline_cache) {
        SerializeVulkanPipelineCache(vulkan_pipeline_cache_filename, vulkan_pipeline_cache,
                                     CACHE_VERSION);
    }

    if (state.statistics) {
        state.statistics->Report();
    }
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
    SPDLOG_INFO("0x{:016x}", key.Hash());
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
    return nullptr;

} catch (const std::exception& exception) {
    SPDLOG_ERROR("{}", exception.what());
    return nullptr;
}

void PipelineCache::SerializeVulkanPipelineCache(const std::filesystem::path& filename,
                                                 const VulkanPipelineCache& pipeline_cache,
                                                 u32 cache_version) {}

auto PipelineCache::LoadVulkanPipelineCache(const std::filesystem::path& filename,
                                            u32 expected_cache_version) -> VulkanPipelineCache {
    return {};
}
auto PipelineCache::createGraphicsPipeline() -> std::unique_ptr<GraphicsPipeline> {
    GraphicsPipelineCacheKey key;
    key.state.color_formats[0] = surface::PixelFormat::B8G8R8A8_UNORM;
    for (size_t i = 1; i < key.state.color_formats.size(); i++) {
        key.state.color_formats[i] = surface::PixelFormat::Invalid;
    }
    key.state.depth_format = surface::PixelFormat::D32_FLOAT;
    key.state.msaa_mode = MsaaMode::Msaa1x1;
    key.state.depth_enabled = 1;
    return createGraphicsPipeline(key, nullptr, false);
}

auto PipelineCache::CreateComputePipeline(const ComputePipelineCacheKey& key)
    -> std::unique_ptr<ComputePipeline> {
    return nullptr;
}

auto PipelineCache::CreateComputePipeline(const ComputePipelineCacheKey& key,
                                          pipeline::PipelineStatistics* statistics,
                                          bool build_in_parallel)
    -> std::unique_ptr<ComputePipeline> {
    return nullptr;
}

}  // namespace render::vulkan