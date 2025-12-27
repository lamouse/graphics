module;
#include "common/file.hpp"
#include <fstream>
#include <memory>
#include <utility>
#include "common/settings.hpp"
#include "render_core/pipeline_state.h"
#include "common/thread_worker.hpp"
#include <xxhash.h>
#include <vulkan/vulkan.hpp>
#include <spdlog/spdlog.h>
module render.vulkan.pipeline_cache;
import render.vulkan.common;
import render.vulkan.shader;
import render.vulkan.scheduler;
import render.vulkan.texture_cache;

import render.vulkan.graphics_pipeline;
import render.vulkan.buffer_cache;
import render.vulkan.update_descriptor;
import render;
import shader;
namespace render::vulkan {

constexpr const char* PIPELINE_CACHE_PATH = "data/cache/pipeline";
constexpr const char* PIPELINE_CACHE_BIN_PATH = "data/cache/pipeline/vulkan.bin";
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
    const u64 hash = XXH64(reinterpret_cast<const char*>(this), sizeof *this, 0);
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
      use_asynchronous_shaders(settings::values.use_asynchronous_shaders.GetValue()),
      use_vulkan_pipeline_cache(settings::values.use_pipeline_cache.GetValue()),
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

    loadPipelineCacheFromDisk();
}

void PipelineCache::loadPipelineCacheFromDisk() {
    if (use_vulkan_pipeline_cache) {
        common::create_dir(PIPELINE_CACHE_PATH);
        std::vector<char> cacheData;

        try {
            // 尝试读取缓存文件
            std::ifstream file(PIPELINE_CACHE_BIN_PATH, std::ios::binary);
            if (file.is_open()) {
                // 读取头部

                file.read(reinterpret_cast<char*>(&pipe_line_cache_header),
                          sizeof(pipe_line_cache_header));
                if (file.gcount() != sizeof(pipe_line_cache_header)) {
                    file.close();
                    throw std::runtime_error("file header error");
                }

                if (pipe_line_cache_header.magic != PipelineCacheHeader::MAGIC ||
                    pipe_line_cache_header.version != CACHE_VERSION) {
                    throw std::runtime_error("cache need update");
                }
                file.seekg(0, std::ios::end);
                size_t totalSize = static_cast<size_t>(file.tellg());
                file.seekg(sizeof(pipe_line_cache_header), std::ios::beg);
                size_t vkDataSize = totalSize - sizeof(PipelineCacheHeader);
                cacheData.resize(vkDataSize);
                file.read(cacheData.data(), vkDataSize);
                file.close();

                vk::PipelineCacheCreateInfo cacheInfo{};
                cacheInfo.initialDataSize = cacheData.size();
                cacheInfo.pInitialData = cacheData.data();
                vulkan_pipeline_cache = device.logical().createPipelineCache(cacheInfo);
            } else {
                VkPipelineCacheCreateInfo cacheInfo{};
                cacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
                vulkan_pipeline_cache = device.logical().createPipelineCache(cacheInfo);
            }
        } catch (const std::exception& e) {
            VkPipelineCacheCreateInfo cacheInfo{};
            cacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
            vulkan_pipeline_cache = device.logical().createPipelineCache(cacheInfo);
        }
    }
}

void PipelineCache::savePipelineCache() {
    if (!use_vulkan_pipeline_cache) {
        return;
    }
    size_t dataSize = 0;

    vulkan_pipeline_cache.Read(&dataSize, nullptr);

    if (dataSize == 0) {
        return;
    }
    std::vector<char> cacheData(dataSize);
    vulkan_pipeline_cache.Read(&dataSize, cacheData.data());
    XXH64_hash_t hash = XXH64(cacheData.data(), dataSize * sizeof(char), 0);
    std::ofstream file(PIPELINE_CACHE_BIN_PATH, std::ios::binary);
    if (file.is_open()) {
        // 构造头部
        PipelineCacheHeader header{};
        header.magic = PipelineCacheHeader::MAGIC;
        header.version = CACHE_VERSION;
        header.hash = hash;
        file.write(reinterpret_cast<const char*>(&header), sizeof(header));
        file.write(cacheData.data(), dataSize);
        file.close();
    }
}

PipelineCache::~PipelineCache() { savePipelineCache(); }
void PipelineCache::updateShaderHash() {
    graphics_key.unique_hashes.at(static_cast<u8>(ShaderType::Vertex)) =
        shader_infos.at(static_cast<u8>(ShaderType::Vertex))->unique_hash;
    graphics_key.unique_hashes.at(static_cast<u8>(ShaderType::Fragment)) =
        shader_infos.at(static_cast<u8>(ShaderType::Fragment))->unique_hash;
}
void PipelineCache::updatePipelineKeyState(PrimitiveTopology topology) {
    graphics_key.state.Refresh(dynamic_features);
    graphics_key.state.topology.Assign(topology);
    graphics_key.state.msaa_mode.Assign(MsaaMode::Msaa1x1);
}
auto PipelineCache::currentGraphicsPipeline(PrimitiveTopology topology) -> GraphicsPipeline* {
    updateShaderHash();
    updatePipelineKeyState(topology);
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

auto PipelineCache::currentComputePipeline(const std::array<u32, 3>& workgroupSize)
    -> ComputePipeline* {
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
        guest_descriptor_queue, thread_worker, render_pass_cache, key, texture_cache, buffer_cache,
        std::move(modules), infos, dynamic_features);
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

    graphics_key.state.depth_enabled.Assign(1);
    return createGraphicsPipeline(graphics_key, false);
}

auto PipelineCache::CreateComputePipeline(const ComputePipelineCacheKey& key)
    -> std::unique_ptr<ComputePipeline> {
    return CreateComputePipeline(key, false);
}

auto PipelineCache::CreateComputePipeline(const ComputePipelineCacheKey& key,
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
                                             guest_descriptor_queue, thread_worker, &shader_notify,
                                             info, std::move(spv_module));
} catch (const std::exception& exception) {
    SPDLOG_ERROR("{}", exception.what());
    return nullptr;
}

}  // namespace render::vulkan