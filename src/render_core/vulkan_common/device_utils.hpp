#pragma once

#include <vulkan/vulkan.hpp>
#include <unordered_map>
#include <set>
// Define all features which may be used by the implementation here.
// Vulkan version in the macro describes the minimum version required for feature availability.
// If the Vulkan version is lower than the required version, the named extension is required.
#define FOR_EACH_VK_FEATURE_1_1(FEATURE)                                               \
    FEATURE(KHR, 16BitStorage, 16BIT_STORAGE, bit16_storage)                           \
    FEATURE(KHR, ShaderAtomicInt64, SHADER_ATOMIC_INT64, shader_atomic_int64)          \
    FEATURE(KHR, ShaderDrawParameters, SHADER_DRAW_PARAMETERS, shader_draw_parameters) \
    FEATURE(KHR, ShaderFloat16Int8, SHADER_FLOAT16_INT8, shader_float16_int8)          \
    FEATURE(KHR, UniformBufferStandardLayout, UNIFORM_BUFFER_STANDARD_LAYOUT,          \
            uniform_buffer_standard_layout)                                            \
    FEATURE(KHR, VariablePointer, VARIABLE_POINTERS, variable_pointer)

#define FOR_EACH_VK_FEATURE_1_2(FEATURE)                             \
    FEATURE(EXT, HostQueryReset, HOST_QUERY_RESET, host_query_reset) \
    FEATURE(KHR, 8BitStorage, 8BIT_STORAGE, bit8_storage)            \
    FEATURE(KHR, TimelineSemaphore, TIMELINE_SEMAPHORE, timeline_semaphore)

#define FOR_EACH_VK_FEATURE_1_3(FEATURE)                                             \
    FEATURE(EXT, ShaderDemoteToHelperInvocation, SHADER_DEMOTE_TO_HELPER_INVOCATION, \
            shader_demote_to_helper_invocation)                                      \
    FEATURE(EXT, SubgroupSizeControl, SUBGROUP_SIZE_CONTROL, subgroup_size_control)

// Define all features which may be used by the implementation and require an extension here.
#define FOR_EACH_VK_FEATURE_EXT(FEATURE)                                                          \
    FEATURE(EXT, CustomBorderColor, CUSTOM_BORDER_COLOR, custom_border_color)                     \
    FEATURE(EXT, DepthBiasControl, DEPTH_BIAS_CONTROL, depth_bias_control)                        \
    FEATURE(EXT, DepthClipControl, DEPTH_CLIP_CONTROL, depth_clip_control)                        \
    FEATURE(EXT, ExtendedDynamicState, EXTENDED_DYNAMIC_STATE, extended_dynamic_state)            \
    FEATURE(EXT, ExtendedDynamicState2, EXTENDED_DYNAMIC_STATE_2, extended_dynamic_state2)        \
    FEATURE(EXT, ExtendedDynamicState3, EXTENDED_DYNAMIC_STATE_3, extended_dynamic_state3)        \
    FEATURE(EXT, 4444Formats, 4444_FORMATS, format_a4b4g4r4)                                      \
    FEATURE(EXT, IndexTypeUint8, INDEX_TYPE_UINT8, index_type_uint8)                              \
    FEATURE(EXT, LineRasterization, LINE_RASTERIZATION, line_rasterization)                       \
    FEATURE(EXT, PrimitiveTopologyListRestart, PRIMITIVE_TOPOLOGY_LIST_RESTART,                   \
            primitive_topology_list_restart)                                                      \
    FEATURE(EXT, ProvokingVertex, PROVOKING_VERTEX, provoking_vertex)                             \
    FEATURE(EXT, Robustness2, ROBUSTNESS_2, robustness2)                                          \
    FEATURE(EXT, TransformFeedback, TRANSFORM_FEEDBACK, transform_feedback)                       \
    FEATURE(EXT, VertexInputDynamicState, VERTEX_INPUT_DYNAMIC_STATE, vertex_input_dynamic_state) \
    FEATURE(KHR, PipelineExecutableProperties, PIPELINE_EXECUTABLE_PROPERTIES,                    \
            pipeline_executable_properties)                                                       \
    FEATURE(KHR, WorkgroupMemoryExplicitLayout, WORKGROUP_MEMORY_EXPLICIT_LAYOUT,                 \
            workgroup_memory_explicit_layout)

// Define miscellaneous extensions which may be used by the implementation here.
#define FOR_EACH_VK_EXTENSION(EXTENSION)                                       \
    EXTENSION(EXT, CONDITIONAL_RENDERING, conditional_rendering)               \
    EXTENSION(EXT, CONSERVATIVE_RASTERIZATION, conservative_rasterization)     \
    EXTENSION(EXT, DEPTH_RANGE_UNRESTRICTED, depth_range_unrestricted)         \
    EXTENSION(EXT, MEMORY_BUDGET, memory_budget)                               \
    EXTENSION(EXT, ROBUSTNESS_2, robustness_2)                                 \
    EXTENSION(EXT, SAMPLER_FILTER_MINMAX, sampler_filter_minmax)               \
    EXTENSION(EXT, SHADER_STENCIL_EXPORT, shader_stencil_export)               \
    EXTENSION(EXT, SHADER_VIEWPORT_INDEX_LAYER, shader_viewport_index_layer)   \
    EXTENSION(EXT, TOOLING_INFO, tooling_info)                                 \
    EXTENSION(EXT, VERTEX_ATTRIBUTE_DIVISOR, vertex_attribute_divisor)         \
    EXTENSION(EXT, EXTERNAL_MEMORY_HOST, external_memory_host)         \
    EXTENSION(KHR, DRAW_INDIRECT_COUNT, draw_indirect_count)                   \
    EXTENSION(KHR, DRIVER_PROPERTIES, driver_properties)                       \
    EXTENSION(KHR, PUSH_DESCRIPTOR, push_descriptor)                           \
    EXTENSION(KHR, SAMPLER_MIRROR_CLAMP_TO_EDGE, sampler_mirror_clamp_to_edge) \
    EXTENSION(KHR, SHADER_FLOAT_CONTROLS, shader_float_controls)               \
    EXTENSION(KHR, SPIRV_1_4, spirv_1_4)                                       \
    EXTENSION(KHR, SWAPCHAIN, swapchain)                                       \
    EXTENSION(KHR, SWAPCHAIN_MUTABLE_FORMAT, swapchain_mutable_format)         \
    EXTENSION(KHR, IMAGE_FORMAT_LIST, image_format_list)                       \
    EXTENSION(NV, DEVICE_DIAGNOSTICS_CONFIG, device_diagnostics_config)        \
    EXTENSION(NV, GEOMETRY_SHADER_PASSTHROUGH, geometry_shader_passthrough)    \
    EXTENSION(NV, VIEWPORT_ARRAY2, viewport_array2)                            \
    EXTENSION(NV, VIEWPORT_SWIZZLE, viewport_swizzle)

// Define extensions which must be supported.
#define FOR_EACH_VK_MANDATORY_EXTENSION(EXTENSION_NAME)                \
    EXTENSION_NAME(VK_EXT_VERTEX_ATTRIBUTE_DIVISOR_EXTENSION_NAME)     \
    EXTENSION_NAME(VK_KHR_DRIVER_PROPERTIES_EXTENSION_NAME)            \
    EXTENSION_NAME(VK_KHR_SAMPLER_MIRROR_CLAMP_TO_EDGE_EXTENSION_NAME) \
    EXTENSION_NAME(VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME)

// Define extensions where the absence of the extension may result in a degraded experience.
#define FOR_EACH_VK_RECOMMENDED_EXTENSION(EXTENSION_NAME)            \
    EXTENSION_NAME(VK_EXT_CONDITIONAL_RENDERING_EXTENSION_NAME)      \
    EXTENSION_NAME(VK_EXT_CONSERVATIVE_RASTERIZATION_EXTENSION_NAME) \
    EXTENSION_NAME(VK_EXT_DEPTH_BIAS_CONTROL_EXTENSION_NAME)         \
    EXTENSION_NAME(VK_EXT_DEPTH_RANGE_UNRESTRICTED_EXTENSION_NAME)   \
    EXTENSION_NAME(VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME)     \
    EXTENSION_NAME(VK_EXT_EXTENDED_DYNAMIC_STATE_2_EXTENSION_NAME)   \
    EXTENSION_NAME(VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME)   \
    EXTENSION_NAME(VK_EXT_EXTERNAL_MEMORY_HOST_EXTENSION_NAME)       \
    EXTENSION_NAME(VK_EXT_4444_FORMATS_EXTENSION_NAME)               \
    EXTENSION_NAME(VK_EXT_LINE_RASTERIZATION_EXTENSION_NAME)         \
    EXTENSION_NAME(VK_EXT_ROBUSTNESS_2_EXTENSION_NAME)               \
    EXTENSION_NAME(VK_EXT_VERTEX_INPUT_DYNAMIC_STATE_EXTENSION_NAME) \
    EXTENSION_NAME(VK_NV_GEOMETRY_SHADER_PASSTHROUGH_EXTENSION_NAME) \
    EXTENSION_NAME(VK_NV_VIEWPORT_ARRAY2_EXTENSION_NAME)             \
    EXTENSION_NAME(VK_NV_VIEWPORT_SWIZZLE_EXTENSION_NAME)

// Define features which must be supported.
#define FOR_EACH_VK_MANDATORY_FEATURE(FEATURE_NAME)                                  \
    FEATURE_NAME(bit16_storage, storageBuffer16BitAccess)                            \
    FEATURE_NAME(bit16_storage, uniformAndStorageBuffer16BitAccess)                  \
    FEATURE_NAME(bit8_storage, storageBuffer8BitAccess)                              \
    FEATURE_NAME(bit8_storage, uniformAndStorageBuffer8BitAccess)                    \
    FEATURE_NAME(features, depthBiasClamp)                                           \
    FEATURE_NAME(features, depthClamp)                                               \
    FEATURE_NAME(features, drawIndirectFirstInstance)                                \
    FEATURE_NAME(features, dualSrcBlend)                                             \
    FEATURE_NAME(features, fillModeNonSolid)                                         \
    FEATURE_NAME(features, fragmentStoresAndAtomics)                                 \
    FEATURE_NAME(features, geometryShader)                                           \
    FEATURE_NAME(features, imageCubeArray)                                           \
    FEATURE_NAME(features, independentBlend)                                         \
    FEATURE_NAME(features, largePoints)                                              \
    FEATURE_NAME(features, logicOp)                                                  \
    FEATURE_NAME(features, multiDrawIndirect)                                        \
    FEATURE_NAME(features, multiViewport)                                            \
    FEATURE_NAME(features, occlusionQueryPrecise)                                    \
    FEATURE_NAME(features, robustBufferAccess)                                       \
    FEATURE_NAME(features, samplerAnisotropy)                                        \
    FEATURE_NAME(features, sampleRateShading)                                        \
    FEATURE_NAME(features, shaderClipDistance)                                       \
    FEATURE_NAME(features, shaderCullDistance)                                       \
    FEATURE_NAME(features, shaderImageGatherExtended)                                \
    FEATURE_NAME(features, shaderStorageImageWriteWithoutFormat)                     \
    FEATURE_NAME(features, tessellationShader)                                       \
    FEATURE_NAME(features, vertexPipelineStoresAndAtomics)                           \
    FEATURE_NAME(features, wideLines)                                                \
    FEATURE_NAME(host_query_reset, hostQueryReset)                                   \
    FEATURE_NAME(shader_demote_to_helper_invocation, shaderDemoteToHelperInvocation) \
    FEATURE_NAME(shader_draw_parameters, shaderDrawParameters)                       \
    FEATURE_NAME(variable_pointer, variablePointers)                                 \
    FEATURE_NAME(variable_pointer, variablePointersStorageBuffer)

// Define features where the absence of the feature may result in a degraded experience.
#define FOR_EACH_VK_RECOMMENDED_FEATURE(FEATURE_NAME)                                 \
    FEATURE_NAME(custom_border_color, customBorderColors)                             \
    FEATURE_NAME(depth_bias_control, depthBiasControl)                                \
    FEATURE_NAME(depth_bias_control, leastRepresentableValueForceUnormRepresentation) \
    FEATURE_NAME(depth_bias_control, depthBiasExact)                                  \
    FEATURE_NAME(extended_dynamic_state, extendedDynamicState)                        \
    FEATURE_NAME(format_a4b4g4r4, formatA4B4G4R4)                                     \
    FEATURE_NAME(index_type_uint8, indexTypeUint8)                                    \
    FEATURE_NAME(primitive_topology_list_restart, primitiveTopologyListRestart)       \
    FEATURE_NAME(provoking_vertex, provokingVertexLast)                               \
    FEATURE_NAME(robustness2, nullDescriptor)                                         \
    FEATURE_NAME(robustness2, robustBufferAccess2)                                    \
    FEATURE_NAME(robustness2, robustImageAccess2)                                     \
    FEATURE_NAME(shader_float16_int8, shaderFloat16)                                  \
    FEATURE_NAME(shader_float16_int8, shaderInt8)                                     \
    FEATURE_NAME(timeline_semaphore, timelineSemaphore)                               \
    FEATURE_NAME(transform_feedback, transformFeedback)                               \
    FEATURE_NAME(uniform_buffer_standard_layout, uniformBufferStandardLayout)         \
    FEATURE_NAME(vertex_input_dynamic_state, vertexInputDynamicState)
namespace render::vulkan::utils {
auto GetFormatProperties(vk::PhysicalDevice physical)
    -> std::unordered_map<vk::Format, vk::FormatProperties>;
enum class NvidiaArchitecture {
    Arch_KeplerOrOlder,
    Arch_Maxwell,
    Arch_Pascal,
    Arch_Volta,
    Arch_Turing,
    Arch_AmpereOrNewer
};
// Misc features
struct MiscFeatures {
        bool is_optimal_astc_supported{};           ///< Support for all guest ASTC formats.
        bool is_blit_depth24_stencil8_supported{};  ///< Support for blitting from and to D24S8.
        bool is_blit_depth32_stencil8_supported{};  ///< Support for blitting from and to D32S8.
        bool is_warp_potentially_bigger{};          ///< Host warp size can be bigger than guest.
        bool is_integrated{};                       ///< Is GPU an iGPU.
        bool is_virtual{};                          ///< Is GPU a virtual GPU.
        bool is_non_gpu{};                     ///< Is SoftwareRasterizer, FPGA, non-GPU device.
        bool has_broken_compute{};             ///< Compute shaders can cause crashes
        bool has_broken_cube_compatibility{};  ///< Has broken cube compatibility bit
        bool has_broken_parallel_compiling{};  ///< Has broken parallel shader compiling.
        bool has_renderdoc{};                  ///< Has RenderDoc attached
        bool has_nsight_graphics{};            ///< Has Nsight Graphics attached
        bool has_radeon_gpu_profiler{};        ///< Has Radeon GPU Profiler attached.
        bool supports_d24_depth{};             ///< Supports D24 depth buffers.
        bool cant_blit_msaa{};                 ///< Does not support MSAA<->MSAA blitting.
        bool must_emulate_scaled_formats{};    ///< Requires scaled vertex format emulation
        bool must_emulate_bgr565{};            ///< Emulates BGR565 by swizzling RGB565 format.
        bool dynamic_state3_blending{};        ///< Has all blending features of dynamic_state3.
        bool dynamic_state3_enables{};         ///< Has all enables features of dynamic_state3.
        bool supports_conditional_barriers{};  ///< Allows barriers in conditional control flow.
};
/// Format usage descriptor.
enum class FormatType { Linear, Optimal, Buffer };
auto getNvidiaArchitecture(vk::PhysicalDevice physical,
                           const std::set<std::string, std::less<>>& exts) -> NvidiaArchitecture;

auto getFormatFeatures(vk::FormatProperties properties, FormatType format_type)
    -> vk::FormatFeatureFlags;

[[nodiscard]] auto checkBrokenCompute(vk::DriverId driver_id, uint32_t driver_version) -> bool;

auto extensionListForVulkan(const std::set<std::string, std::less<>>& extensions)
    -> std::vector<const char*>;

class VulkanException final : public std::exception {
    public:
        /// Construct the exception with a result.
        /// @pre result != VK_SUCCESS
        explicit VulkanException(vk::Result result_) : result{result_} {}
        ~VulkanException() override = default;

        [[nodiscard]] auto what() const noexcept -> const char* override;
        [[nodiscard]] auto getResult() const noexcept -> vk::Result { return result; }

    private:
        vk::Result result;
};

}  // namespace render::vulkan::utils