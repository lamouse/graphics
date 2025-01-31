#include "pipeline_cache.hpp"
#include "common/microprofile.hpp"
#include "common/cityhash.h"
#include "common/fs/fs.h"
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
#include "render_core/host_shaders/model_vert_spv.h"
#include "render_core/host_shaders/model_frag_spv.h"
#include "render_core/render_vulkan/vk_shader_util.hpp"
#include "shader_tools/shader_compile.hpp"

namespace render::vulkan {

namespace {

constexpr std::array<char, 8> MAGIC_NUMBER{'e', 'n', 'g', 'e', 'c', 'a', 'c', 'h'};
void LoadPipelines(std::stop_token stop_loading, const std::filesystem::path& filename,
                   u32 expected_cache_version,
                   common::UniqueFunction<void, std::ifstream&> load_compute,
                   common::UniqueFunction<void, std::ifstream&> load_graphics) try {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        return;
    }
    file.exceptions(std::ifstream::failbit);
    const auto end{file.tellg()};
    file.seekg(0, std::ios::beg);

    std::array<char, 8> magic_number;
    u32 cache_version;
    file.read(magic_number.data(), magic_number.size())
        .read(reinterpret_cast<char*>(&cache_version), sizeof(cache_version));
    if (magic_number != MAGIC_NUMBER || cache_version != expected_cache_version) {
        file.close();
        if (common::FS::RemoveFile(filename)) {
            if (magic_number != MAGIC_NUMBER) {
                SPDLOG_ERROR("Invalid pipeline cache file");
            }
            if (cache_version != expected_cache_version) {
                SPDLOG_INFO("Deleting old pipeline cache");
            }
        } else {
            SPDLOG_ERROR("Invalid pipeline cache file and failed to delete it in \"{}\"",
                         common::FS::PathToUTF8String(filename));
        }
        return;
    }
    while (file.tellg() != end) {
        if (stop_loading.stop_requested()) {
            return;
        }
        u32 num_envs{};
        file.read(reinterpret_cast<char*>(&num_envs), sizeof(num_envs));
        // std::vector<FileEnvironment> envs(num_envs);
        // for (FileEnvironment& env : envs) {
        //     env.Deserialize(file);
        // }
        // if (envs.front().ShaderStage() == shader::Stage::Compute) {
        //     load_compute(file, std::move(envs.front()));
        // } else {
        //     load_graphics(file, std::move(envs));
        // }
    }

} catch (const std::ios_base::failure& e) {
    SPDLOG_ERROR("{}", e.what());
    if (!common::FS::RemoveFile(filename)) {
        SPDLOG_ERROR("Failed to delete pipeline cache file {}",
                     common::FS::PathToUTF8String(filename));
    }
}

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
    const u64 hash = common::CityHash64(reinterpret_cast<const char*>(this), sizeof *this);
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
    const auto& float_control = device.FloatControlProperties();
    const vk::DriverId driver_id = device.getDriverID();
    profile = Shader::Profile{
        .supported_spirv = device.SupportedSpirvVersion(),
        .unified_descriptor_binding = true,
        .support_descriptor_aliasing = device.IsDescriptorAliasingSupported(),
        .support_int8 = device.IsInt8Supported(),
        .support_int16 = device.IsShaderInt16Supported(),
        .support_int64 = device.IsShaderInt64Supported(),
        .support_vertex_instance_id = false,
        .support_float_controls = device.IsKhrShaderFloatControlsSupported(),
        .support_separate_denorm_behavior =
            float_control.denormBehaviorIndependence == VK_SHADER_FLOAT_CONTROLS_INDEPENDENCE_ALL,
        .support_separate_rounding_mode =
            float_control.roundingModeIndependence == VK_SHADER_FLOAT_CONTROLS_INDEPENDENCE_ALL,
        .support_fp16_denorm_preserve = float_control.shaderDenormPreserveFloat16 != VK_FALSE,
        .support_fp32_denorm_preserve = float_control.shaderDenormPreserveFloat32 != VK_FALSE,
        .support_fp16_denorm_flush = float_control.shaderDenormFlushToZeroFloat16 != VK_FALSE,
        .support_fp32_denorm_flush = float_control.shaderDenormFlushToZeroFloat32 != VK_FALSE,
        .support_fp16_signed_zero_nan_preserve =
            float_control.shaderSignedZeroInfNanPreserveFloat16 != VK_FALSE,
        .support_fp32_signed_zero_nan_preserve =
            float_control.shaderSignedZeroInfNanPreserveFloat32 != VK_FALSE,
        .support_fp64_signed_zero_nan_preserve =
            float_control.shaderSignedZeroInfNanPreserveFloat64 != VK_FALSE,
        .support_explicit_workgroup_layout = device.IsKhrWorkgroupMemoryExplicitLayoutSupported(),
        .support_vote = device.IsSubgroupFeatureSupported(VK_SUBGROUP_FEATURE_VOTE_BIT),
        .support_viewport_index_layer_non_geometry =
            device.IsExtShaderViewportIndexLayerSupported(),
        .support_viewport_mask = device.IsNvViewportArray2Supported(),
        .support_typeless_image_loads = device.IsFormatlessImageLoadSupported(),
        .support_demote_to_helper_invocation =
            device.IsExtShaderDemoteToHelperInvocationSupported(),
        .support_int64_atomics = device.IsExtShaderAtomicInt64Supported(),
        .support_derivative_control = true,
        .support_geometry_shader_passthrough = device.IsNvGeometryShaderPassthroughSupported(),
        .support_native_ndc = device.IsExtDepthClipControlSupported(),
        .support_scaled_attributes = !device.MustEmulateScaledFormats(),
        .support_multi_viewport = device.SupportsMultiViewport(),
        .support_geometry_streams = device.AreTransformFeedbackGeometryStreamsSupported(),

        .warp_size_potentially_larger_than_guest = device.IsWarpSizePotentiallyBiggerThanGuest(),

        .lower_left_origin_mode = false,
        .need_declared_frag_colors = false,
        .need_gather_subpixel_offset = driver_id == vk::DriverIdKHR::eAmdProprietary ||
                                       driver_id == vk::DriverIdKHR::eAmdOpenSource ||
                                       driver_id == vk::DriverIdKHR::eMesaRadv ||
                                       driver_id == vk::DriverIdKHR::eIntelProprietaryWindows ||
                                       driver_id == vk::DriverIdKHR::eIntelOpenSourceMESA,

        .has_broken_spirv_clamp = driver_id == vk::DriverIdKHR::eIntelProprietaryWindows,
        .has_broken_spirv_position_input = driver_id == vk::DriverIdKHR::eQualcommProprietary,
        .has_broken_unsigned_image_offsets = false,
        .has_broken_signed_operations = false,
        .has_broken_fp16_float_controls = driver_id == vk::DriverIdKHR::eNvidiaProprietary,
        .ignore_nan_fp_comparisons = false,
        .has_broken_spirv_subgroup_mask_vector_extract_dynamic =
            driver_id == vk::DriverIdKHR::eQualcommProprietary,
        .has_broken_robust =
            device.IsNvidia() && device.GetNvidiaArch() <= utils::NvidiaArchitecture::Arch_Pascal,
        .min_ssbo_alignment = device.GetStorageBufferAlignment(),
        .max_user_clip_distances = device.GetMaxUserClipDistances(),
    };

    host_info = Shader::HostTranslateInfo{
        .support_float64 = device.IsFloat64Supported(),
        .support_float16 = device.IsFloat16Supported(),
        .support_int64 = device.IsShaderInt64Supported(),
        .needs_demote_reorder = driver_id == vk::DriverIdKHR::eAmdProprietary ||
                                driver_id == vk::DriverIdKHR::eAmdOpenSource ||
                                driver_id == vk::DriverIdKHR::eSamsungProprietary,
        .support_snorm_render_buffer = true,
        .support_viewport_index_layer = device.IsExtShaderViewportIndexLayerSupported(),
        .min_ssbo_alignment = static_cast<u32>(device.GetStorageBufferAlignment()),
        .support_geometry_shader_passthrough = device.IsNvGeometryShaderPassthroughSupported(),
        .support_conditional_barrier = device.SupportsConditionalBarriers(),
    };

    dynamic_features = DynamicFeatures{
        .has_extended_dynamic_state = device.IsExtExtendedDynamicStateSupported(),
        .has_extended_dynamic_state_2 = device.IsExtExtendedDynamicState2Supported(),
        .has_extended_dynamic_state_2_extra = device.IsExtExtendedDynamicState2ExtrasSupported(),
        .has_extended_dynamic_state_3_blend = device.IsExtExtendedDynamicState3BlendingSupported(),
        .has_extended_dynamic_state_3_enables = device.IsExtExtendedDynamicState3EnablesSupported(),
        .has_dynamic_vertex_input = device.IsExtVertexInputDynamicStateSupported(),
    };

    createGraphicsPipeline();  // TODO 临时设置一下后续使用
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
    LoadPipelines(stop_loading, pipeline_cache_filename, CACHE_VERSION, load_compute,
                  load_graphics);

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
    auto hash = key.Hash();
    SPDLOG_INFO("0x{:016x}", hash);
    size_t env_index{0};
    std::array<ShaderModule, 5> modules;
    modules[0] = utils::buildShader(device.getLogical(), MODEL_VERT_SPV);
    modules[4] = utils::buildShader(device.getLogical(), MODEL_FRAG_SPV);
    std::array<const shader::Info*, 5> infos{};
    shader::Info vertex_info = shader::compile::getShaderInfo(MODEL_VERT_SPV);
    shader::Info frag_info = shader::compile::getShaderInfo(MODEL_FRAG_SPV);
    infos[0] = &vertex_info;
    infos[4] = &frag_info;
    common::ThreadWorker* const thread_worker{build_in_parallel ? &workers : nullptr};
    auto pipeline = std::make_unique<GraphicsPipeline>(
        scheduler, vulkan_pipeline_cache, &shader_notify, device, descriptor_pool, guest_descriptor_queue,
        thread_worker, statistics, render_pass_cache, key, texture_cache, std::move(modules), infos,
        dynamic_features);
    current_pipeline = pipeline.get();
    if (pipeline) {
        graphics_cache.emplace(key, std::move(pipeline));
    }
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
    for (int i = 1; i < key.state.color_formats.size(); i++) {
        key.state.color_formats[i] = surface::PixelFormat::Invalid;
    }
    key.state.depth_format = surface::PixelFormat::D32_FLOAT;
    key.state.msaa_mode = MsaaMode::Msaa1x1;
    key.state.depth_enabled = 0;
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