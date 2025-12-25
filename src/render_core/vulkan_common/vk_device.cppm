module;
#include <unordered_map>
#include <set>
#include "render_core/surface.hpp"
#include "vk_device_feature.hpp"
#include <vulkan/vulkan.hpp>
#include "vma.hpp"

export module render.vulkan.common.driver;

import render.vulkan.common.wrapper;
import render.vulkan.common.device.utils;


export namespace render::vulkan {
struct FormatInfo {
        vk::Format format;
        bool attachable;
        bool storage;
};
/// Format usage descriptor.
 enum class FormatType { Linear, Optimal, Buffer };

/// Subgroup size of the guest emulated hardware (Nvidia has 32 threads per subgroup).
const uint32_t GUEST_WARP_SIZE = 32;
class Device {
    public:
        explicit Device(vk::Instance instance, vk::PhysicalDevice physical, vk::SurfaceKHR surface);
        ~Device();
        [[nodiscard]] auto getSetsPerPool() const -> uint32_t { return sets_per_pool_; }
        /// Returns the logical device.
        [[nodiscard]] auto getLogical() const -> const vk::Device { return *logical_; }
        [[nodiscard]] auto logical() const -> const LogicDevice& { return logical_; }
        /// Returns the physical device.
        [[nodiscard]] auto getPhysical() const -> vk::PhysicalDevice { return physical_; }

        /// Returns the main graphics queue.
        [[nodiscard]] auto getGraphicsQueue() const -> vk::Queue { return graphics_queue_; }

        /// Returns the main present queue.
        [[nodiscard]] auto getPresentQueue() const -> vk::Queue { return present_queue_; }
        /// Returns the main compute queue.
        [[nodiscard]] auto getComputeQueue() const -> vk::Queue { return compute_queue_; }

        /// Returns main graphics queue family index.
        [[nodiscard]] auto getGraphicsFamily() const -> uint32_t { return graphics_family_; }

        /// Returns main present queue family index.
        [[nodiscard]] auto getPresentFamily() const -> uint32_t { return present_family_; }

        /// Returns main compute queue family index.
        [[nodiscard]] auto getComputeFamily() const -> uint32_t { return compute_family_; }
        [[nodiscard]] auto getInstanceVersion() const -> uint32_t { return instance_version_; }
        /// Returns true if VK_KHR_swapchain_mutable_format is enabled.
        [[nodiscard]] auto isKhrSwapchainMutableFormatEnabled() const -> bool {
            return extensions_.swapchain_mutable_format;
        }
        /// Returns true if the device supports VK_EXT_sampler_filter_minmax.
        [[nodiscard]] auto isExtSamplerFilterMinmaxSupported() const -> bool {
            return extensions_.sampler_filter_minmax;
        }

        /// Returns true if the device supports VK_EXT_subgroup_size_control.
        [[nodiscard]] auto IsExtSubgroupSizeControlSupported() const -> bool {
            return extensions_.subgroup_size_control;
        }

        [[nodiscard]] auto mustEmulateScaledFormats() const -> bool {
            return misc_features_.must_emulate_scaled_formats;
        }

        [[nodiscard]] auto mustEmulateBGR565() const -> bool {
            return misc_features_.must_emulate_bgr565;
        }
        [[nodiscard]] auto hasNullDescriptor() const -> bool {
            return features_.robustness2.nullDescriptor;
        }

        [[nodiscard]] auto getMaxAnisotropy() const -> float {
            return properties_.properties_.limits.maxSamplerAnisotropy;
        }

        [[nodiscard]] auto cantBlitMSAA() const -> bool { return misc_features_.cant_blit_msaa; }

        /// Returns true if the device supports VK_KHR_push_descriptor.
        [[nodiscard]] auto isKhrPushDescriptorSupported() const -> bool {
            return extensions_.push_descriptor;
        }
        [[nodiscard]] auto hasTimelineSemaphore() const -> bool;
        /// Returns the driver ID.
        [[nodiscard]] auto getDriverID() const -> vk::DriverIdKHR {
            return static_cast<vk::DriverIdKHR>(properties_.driver_.driverID);
        }
        /// Returns the VMA allocator.
        [[nodiscard]] auto getAllocator() const -> VmaAllocator { return allocator_; }
        /// Returns true when a known debugging tool is attached.
        [[nodiscard]] auto hasDebuggingToolAttached() const -> bool {
            return misc_features_.has_renderdoc || misc_features_.has_nsight_graphics ||
                   misc_features_.has_radeon_gpu_profiler;
        }
        /// Returns the maximum number of push descriptors.
        [[nodiscard]] auto maxPushDescriptors() const -> u32 {
            return properties_.push_descriptor_.maxPushDescriptors;
        }
        auto GetMaxVertexInputAttributes() const -> u32 {
            return properties_.properties_.limits.maxVertexInputAttributes;
        }
        /// Returns true if the device supports VK_EXT_transform_feedback.
        [[nodiscard]] auto isExtTransformFeedbackSupported() const -> bool {
            return extensions_.transform_feedback;
        }
        /// Returns true if the device supports VK_EXT_subgroup_size_control.
        [[nodiscard]] auto isExtSubgroupSizeControlSupported() const -> bool {
            return extensions_.subgroup_size_control;
        }  /// Returns true if the device supports binding multisample images as storage images.
        [[nodiscard]] auto isStorageImageMultisampleSupported() const -> bool {
            return features_.features.shaderStorageImageMultisample;
        }
        /// Returns true if the device supports VK_KHR_image_format_list.
        [[nodiscard]] auto isKhrImageFormatListSupported() const -> bool {
            return extensions_.image_format_list || instance_version_ >= VK_API_VERSION_1_2;
        }

        /// Returns true if VK_KHR_shader_float_controls is enabled.
        [[nodiscard]] auto IsKhrShaderFloatControlsSupported() const -> bool {
            return extensions_.shader_float_controls;
        }

        /// Returns true if ASTC is natively supported.
        [[nodiscard]] auto isOptimalAstcSupported() const -> bool {
            return features_.features.textureCompressionASTC_LDR;
        }

        /// Returns true if BCn is natively supported.
        [[nodiscard]] auto isOptimalBcnSupported() const -> bool {
            return features_.features.textureCompressionBC;
        }
        /// Returns true when blitting from and to D24S8 images is supported.
        [[nodiscard]] auto isBlitDepth24Stencil8Supported() const -> bool {
            return misc_features_.is_blit_depth24_stencil8_supported;
        }
        /// Returns true if the device supports VK_EXT_custom_border_color.

        /// Returns true when blitting from and to D32S8 images is supported.
        [[nodiscard]] auto isBlitDepth32Stencil8Supported() const -> bool {
            return misc_features_.is_blit_depth32_stencil8_supported;
        }

        /// Returns true if the device supports VK_EXT_custom_border_color.
        [[nodiscard]] auto isExtCustomBorderColorSupported() const -> bool {
            return extensions_.custom_border_color;
        }

        /// Returns true if parallel shader compiling has issues with the current driver.
        [[nodiscard]] auto hasBrokenParallelShaderCompiling() const -> bool {
            return misc_features_.has_broken_parallel_compiling;
        }

        /// Returns true if the device supports VK_EXT_primitive_topology_list_restart.
        [[nodiscard]] auto IsTopologyListPrimitiveRestartSupported() const -> bool {
            return features_.primitive_topology_list_restart.primitiveTopologyListRestart;
        }

        /// Returns true if the device supports VK_EXT_primitive_topology_list_restart.
        [[nodiscard]] auto IsPatchListPrimitiveRestartSupported() const -> bool {
            return features_.primitive_topology_list_restart.primitiveTopologyPatchListRestart;
        }

        /// Returns true if the device supports VK_EXT_line_rasterization.
        [[nodiscard]] auto IsExtLineRasterizationSupported() const -> bool {
            return extensions_.line_rasterization;
        }

        /// Returns true if the device supports VK_EXT_conservative_rasterization.
        [[nodiscard]] auto IsExtConservativeRasterizationSupported() const -> bool {
            return extensions_.conservative_rasterization;
        }

        /// Returns true if the device supports VK_EXT_provoking_vertex.
        [[nodiscard]] auto IsExtProvokingVertexSupported() const -> bool {
            return extensions_.provoking_vertex;
        }
        // Returns true if depth bounds is supported.
        [[nodiscard]] auto IsDepthBoundsSupported() const -> bool {
            return features_.features.depthBounds;
        }

        /// Returns the minimum supported version of SPIR-V.
        [[nodiscard]] auto SupportedSpirvVersion() const -> u32 {
            if (instance_version_ >= VK_API_VERSION_1_3) {
                return 0x00010600U;
            }
            if (extensions_.spirv_1_4) {
                return 0x00010400U;
            }
            return 0x00010300U;
        }

        /// Returns true if descriptor aliasing is natively supported.
        [[nodiscard]] auto IsDescriptorAliasingSupported() const -> bool {
            return getDriverID() != vk::DriverId::eQualcommProprietary;
        }

        /// Returns true if the device supports float64 natively.
        [[nodiscard]] auto IsFloat64Supported() const -> bool {
            return features_.features.shaderFloat64;
        }

        /// Returns true if the device supports float16 natively.
        [[nodiscard]] auto IsFloat16Supported() const -> bool {
            return features_.shader_float16_int8.shaderFloat16;
        }

        /// Returns true if the device supports int8 natively.
        [[nodiscard]] auto IsInt8Supported() const -> bool {
            return features_.shader_float16_int8.shaderInt8;
        }

        /// Returns true if shader int64 is supported.
        [[nodiscard]] auto IsShaderInt64Supported() const -> bool {
            return features_.features.shaderInt64;
        }

        /// Returns true if shader int16 is supported.
        [[nodiscard]] auto IsShaderInt16Supported() const -> bool {
            return features_.features.shaderInt16;
        }
        /// @returns True if compute pipelines can cause crashing.
        [[nodiscard]] auto HasBrokenCompute() const -> bool {
            return misc_features_.has_broken_compute;
        }

        [[nodiscard]] auto GetMaxViewports() const -> u32 {
            return properties_.properties_.limits.maxViewports;
        }

        [[nodiscard]] auto getDeviceLocalMemory() const -> u64 { return device_access_memory_; }

        [[nodiscard]] auto getDeviceMemoryUsage() const -> u64;
        [[nodiscard]] auto getDriverName() const -> std::string;
        [[nodiscard]] auto canReportMemoryUsage() const -> bool {
            return extensions_.memory_budget;
        }

        /// Checks if we are running MolvenVK.
        [[nodiscard]] auto isMoltenVK() const noexcept -> bool {
            return properties_.driver_.driverID == vk::DriverId::eMoltenvk;
        }
        /// Returns uniform buffer alignment requirement.
        [[nodiscard]] auto GetUniformBufferAlignment() const -> VkDeviceSize {
            return properties_.properties_.limits.minUniformBufferOffsetAlignment;
        }
        /// Reports a device loss.
        void reportLoss() const;
        /// Returns true if the device supports VK_EXT_shader_stencil_export.
        [[nodiscard]] auto isExtShaderStencilExportSupported() const -> bool {
            return extensions_.shader_stencil_export;
        }

        /// Returns true when the device does not properly support cube compatibility.
        [[nodiscard]] auto hasBrokenCubeImageCompatibility() const -> bool {
            return misc_features_.has_broken_cube_compatibility;
        }
        /// Returns float control properties of the device.
        [[nodiscard]] auto FloatControlProperties() const
            -> const VkPhysicalDeviceFloatControlsPropertiesKHR& {
            return properties_.float_controls_;
        }

        /// Returns true if the device supports VK_KHR_workgroup_memory_explicit_layout.
        [[nodiscard]] auto IsKhrWorkgroupMemoryExplicitLayoutSupported() const -> bool {
            return extensions_.workgroup_memory_explicit_layout;
        }
        /// Returns true if the device supports the provided subgroup feature.
        [[nodiscard]] auto IsSubgroupFeatureSupported(vk::SubgroupFeatureFlags feature) const
            -> bool {
            return (properties_.subgroup_properties_.supportedOperations & feature) !=
                   vk::SubgroupFeatureFlags{};
        }

        /// Returns true if the device supports VK_EXT_shader_viewport_index_layer.
        [[nodiscard]] auto IsExtShaderViewportIndexLayerSupported() const -> bool {
            return extensions_.shader_viewport_index_layer;
        }
        /// Returns true if formatless image load is supported.
        [[nodiscard]] auto IsFormatlessImageLoadSupported() const -> bool {
            return features_.features.shaderStorageImageReadWithoutFormat;
        }

        /// Returns true if the device supports VK_EXT_shader_demote_to_helper_invocation
        [[nodiscard]] auto IsExtShaderDemoteToHelperInvocationSupported() const -> bool {
            return extensions_.shader_demote_to_helper_invocation;
        }

        /// Returns true if the device supports VK_KHR_shader_atomic_int64.
        [[nodiscard]] auto IsExtShaderAtomicInt64Supported() const -> bool {
            return extensions_.shader_atomic_int64;
        }

        /// Returns true if the device supports VK_EXT_depth_clip_control.
        [[nodiscard]] auto IsExtDepthClipControlSupported() const -> bool {
            return extensions_.depth_clip_control;
        }
        /// Returns true if the device supports VK_EXT_transform_feedback properly.
        [[nodiscard]] auto AreTransformFeedbackGeometryStreamsSupported() const -> bool {
            return features_.transform_feedback.geometryStreams;
        }

        /// Returns true if the device warp size can potentially be bigger than guest's warp size.
        [[nodiscard]] auto IsWarpSizePotentiallyBiggerThanGuest() const -> bool {
            return misc_features_.is_warp_potentially_bigger;
        }

        /// Returns storage alignment requirement.
        [[nodiscard]] auto GetStorageBufferAlignment() const -> VkDeviceSize {
            return properties_.properties_.limits.minStorageBufferOffsetAlignment;
        }

        /// Returns true if VK_KHR_pipeline_executable_properties is enabled.
        [[nodiscard]] auto IsKhrPipelineExecutablePropertiesEnabled() const -> bool {
            return extensions_.pipeline_executable_properties;
        }

        /// Returns true if the device supports VK_EXT_index_type_uint8.
        [[nodiscard]] auto IsExtIndexTypeUint8Supported() const -> bool {
            return extensions_.index_type_uint8;
        }
        /// Returns true if the device supports VK_EXT_extended_dynamic_state.
        [[nodiscard]] auto IsExtExtendedDynamicStateSupported() const -> bool {
            return extensions_.extended_dynamic_state;
        }
        [[nodiscard]] auto GetMaxUserClipDistances() const -> u32 {
            return properties_.properties_.limits.maxClipDistances;
        }
        /// Returns true if the device supports VK_EXT_transform_feedback.
        [[nodiscard]] auto IsExtTransformFeedbackSupported() const -> bool {
            return extensions_.transform_feedback;
        }
        /// Returns true if the device supports VK_EXT_extended_dynamic_state2.
        [[nodiscard]] auto IsExtExtendedDynamicState2Supported() const -> bool {
            return extensions_.extended_dynamic_state2;
        }

        [[nodiscard]] auto IsFormatSupport(vk::Format format,
                                           vk::FormatFeatureFlagBits feature) const -> bool;

        [[nodiscard]] auto IsExtExtendedDynamicState2ExtrasSupported() const -> bool {
            return features_.extended_dynamic_state2.extendedDynamicState2LogicOp;
        }

        /// Returns true if the device supports VK_EXT_extended_dynamic_state3.
        [[nodiscard]] auto IsExtExtendedDynamicState3Supported() const -> bool {
            return extensions_.extended_dynamic_state3;
        }

        /// Returns true if the device supports VK_EXT_extended_dynamic_state3.
        [[nodiscard]] auto IsExtExtendedDynamicState3BlendingSupported() const -> bool {
            return misc_features_.dynamic_state3_blending;
        }

        /// Returns true if the device supports VK_EXT_extended_dynamic_state3.
        [[nodiscard]] auto IsExtExtendedDynamicState3EnablesSupported() const -> bool {
            return misc_features_.dynamic_state3_enables;
        }

        /// Returns true if the device supports VK_EXT_vertex_input_dynamic_state.
        [[nodiscard]] auto IsExtVertexInputDynamicStateSupported() const -> bool {
            return extensions_.vertex_input_dynamic_state;
        }

        [[nodiscard]] auto IsExtConditionalRendering() const -> bool {
            return extensions_.conditional_rendering;
        }

        [[nodiscard]] auto getMaxVertexInputBindings() const -> u32 {
            return properties_.properties_.limits.maxVertexInputBindings;
        }
        [[nodiscard]] auto MustEmulateScaledFormats() const -> bool {
            return misc_features_.must_emulate_scaled_formats;
        }
        [[nodiscard]] auto SupportsConditionalBarriers() const -> bool {
            return misc_features_.supports_conditional_barriers;
        }

        [[nodiscard]] auto GetNvidiaArch() const noexcept -> utils::NvidiaArchitecture {
            return nvidia_arch;
        }
        [[nodiscard]] auto IsNvidia() const noexcept -> bool {
            return properties_.driver_.driverID == vk::DriverId::eNvidiaProprietary;
        }

        [[nodiscard]] auto surfaceFormat(FormatType format_type, bool with_srgb,
                                         surface::PixelFormat pixel_format) const -> FormatInfo;

        [[nodiscard]] auto getSupportedFormat(vk::Format wanted_format,
                                              vk::FormatFeatureFlags wanted_usage,
                                              FormatType format_type) const -> vk::Format;

        [[nodiscard]] auto isFormatSupported(vk::Format wanted_format,
                                             vk::FormatFeatureFlags wanted_usage,
                                             FormatType format_type) const -> bool;

        [[nodiscard]] auto HasNullDescriptor() const -> bool {
            return features_.robustness2.nullDescriptor;
        }

        /// Returns true if the device supports float16 natively.
        [[nodiscard]] auto isFloat16Supported() const -> bool {
            return features_.shader_float16_int8.shaderFloat16;
        }

    private:
        vk::Instance instance_;
        vk::PhysicalDevice physical_;
        LogicDevice logical_;
        vk::Queue graphics_queue_;
        vk::Queue present_queue_;
        vk::Queue compute_queue_;

        uint32_t graphics_family_;
        uint32_t present_family_ = 0;
        uint32_t compute_family_;
        uint32_t instance_version_{};
        VmaAllocator allocator_;  ///< VMA allocator.
        /// Format properties dictionary.
        std::unordered_map<vk::Format, vk::FormatProperties> format_properties_;
        std::vector<size_t> valid_heap_memory_;  ///< Heaps used.

        struct Properties {
                vk::PhysicalDeviceDriverProperties driver_;
                vk::PhysicalDeviceSubgroupProperties subgroup_properties_;
                vk::PhysicalDeviceFloatControlsProperties float_controls_;
                vk::PhysicalDevicePushDescriptorPropertiesKHR push_descriptor_;
                vk::PhysicalDeviceSubgroupSizeControlProperties subgroup_size_control_;
                vk::PhysicalDeviceTransformFeedbackPropertiesEXT transform_feedback_;

                vk::PhysicalDeviceProperties properties_;
        };
        utils::MiscFeatures misc_features_;
        uint64_t device_access_memory_{};  ///< Total size of device local memory in bytes.
        uint32_t sets_per_pool_{};         ///< Sets per Description Pool
        // Telemetry parameters
        std::set<std::string, std::less<>> supported_extensions_;  ///< Reported Vulkan extensions.
        std::set<std::string, std::less<>> loaded_extensions_;     ///< Loaded Vulkan extensions.
        utils::NvidiaArchitecture nvidia_arch{utils::NvidiaArchitecture::Arch_AmpereOrNewer};
        auto getSuitability(bool requires_swapchain) -> bool;
        void removeUnsuitableExtensions();
        void removeExtension(bool& extension, const std::string& extension_name);
        template <typename Feature>
        void removeExtensionFeatureIfUnsuitable(bool is_suitable, Feature& feature,
                                                const std::string& extension_name);
        template <typename Feature>
        void removeExtensionFeature(bool& extension, Feature& feature,
                                    const std::string& extension_name);
        [[nodiscard]] auto computeIsOptimalAstcSupported() const -> bool;
        void setupFamilies(vk::SurfaceKHR surface);
        [[nodiscard]] auto testDepthStencilBlits(vk::Format format) const -> bool;
        [[nodiscard]] auto getDeviceQueueCreateInfos() const
            -> std::vector<vk::DeviceQueueCreateInfo>;

        void collectPhysicalMemoryInfo();
        void collectToolingInfo();
        struct Extensions {
#define EXTENSION(prefix, macro_name, var_name) bool var_name{};
#define FEATURE(prefix, struct_name, macro_name, var_name) bool var_name{};

                FOR_EACH_VK_FEATURE_1_1(FEATURE);
                FOR_EACH_VK_FEATURE_1_2(FEATURE);
                FOR_EACH_VK_FEATURE_1_3(FEATURE);
                FOR_EACH_VK_FEATURE_EXT(FEATURE);
                FOR_EACH_VK_EXTENSION(EXTENSION);

#undef EXTENSION
#undef FEATURE
        };

        struct Features {
#define FEATURE_CORE(prefix, struct_name, macro_name, var_name) \
    vk::PhysicalDevice##struct_name##Features var_name;
#define FEATURE_EXT(prefix, struct_name, macro_name, var_name) \
    vk::PhysicalDevice##struct_name##Features##prefix var_name;

                FOR_EACH_VK_FEATURE_1_1(FEATURE_CORE);
                FOR_EACH_VK_FEATURE_1_2(FEATURE_CORE);
                FOR_EACH_VK_FEATURE_1_3(FEATURE_CORE);
                FOR_EACH_VK_FEATURE_EXT(FEATURE_EXT);

#undef FEATURE_CORE
#undef FEATURE_EXT
                vk::PhysicalDeviceFeatures features{};
        };
        Extensions extensions_{};
        Properties properties_;
        Features features_{};
        vk::PhysicalDeviceFeatures2 features2_;
        vk::PhysicalDeviceProperties2 properties2_;
};

}  // namespace render::vulkan