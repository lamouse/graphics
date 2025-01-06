#pragma once
#include <vulkan/vulkan.hpp>
#include "device_utils.hpp"
#include <unordered_map>
#include <set>
#include "vma.hpp"
#include "vulkan_wrapper.hpp"
#include "render_core/surface.hpp"
/**
 * @brief vulkan device
 *
 */
VK_DEFINE_HANDLE(VmaAllocator)
namespace render::vulkan {
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
        [[nodiscard]] auto tryAllocateMemory(const VkMemoryAllocateInfo& ai) const noexcept
            -> DeviceMemory;
        [[nodiscard]] auto createDescriptorPool(const vk::DescriptorPoolCreateInfo& ci) const
            -> VulkanDescriptorPool;
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
        /// Returns true if ASTC is natively supported.
        [[nodiscard]] auto isOptimalAstcSupported() const -> bool {
            return features_.features.textureCompressionASTC_LDR;
        }

        /// Returns true if BCn is natively supported.
        [[nodiscard]] auto isOptimalBcnSupported() const -> bool {
            return features_.features.textureCompressionBC;
        }

        /// Reports a device loss.
        void reportLoss() const;

        [[nodiscard]] auto createDescriptorSetLayout(const vk::DescriptorSetLayoutCreateInfo&) const
            -> DescriptorSetLayout;
        [[nodiscard]] auto createPipelineLayout(const vk::PipelineLayoutCreateInfo& ci) const
            -> PipelineLayout;
        /// Returns true if the device supports VK_EXT_shader_stencil_export.
        [[nodiscard]] auto isExtShaderStencilExportSupported() const -> bool {
            return extensions_.shader_stencil_export;
        }
        auto surfaceFormat(FormatType format_type, bool with_srgb,
                           surface::PixelFormat pixel_format) const -> FormatInfo;

        [[nodiscard]] auto getSupportedFormat(vk::Format wanted_format,
                                              vk::FormatFeatureFlags wanted_usage,
                                              FormatType format_type) const -> vk::Format;

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
                VkPhysicalDeviceDriverProperties driver_;
                VkPhysicalDeviceSubgroupProperties subgroup_properties_;
                VkPhysicalDeviceFloatControlsProperties float_controls_;
                VkPhysicalDevicePushDescriptorPropertiesKHR push_descriptor_;
                VkPhysicalDeviceSubgroupSizeControlProperties subgroup_size_control_;
                VkPhysicalDeviceTransformFeedbackPropertiesEXT transform_feedback_;

                VkPhysicalDeviceProperties properties_;
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
        [[nodiscard]] auto isFormatSupported(vk::Format wanted_format,
                                             vk::FormatFeatureFlags wanted_usage,
                                             FormatType format_type) const -> bool;
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
    VkPhysicalDevice##struct_name##Features var_name{};
#define FEATURE_EXT(prefix, struct_name, macro_name, var_name) \
    VkPhysicalDevice##struct_name##Features##prefix var_name{};

                FOR_EACH_VK_FEATURE_1_1(FEATURE_CORE);
                FOR_EACH_VK_FEATURE_1_2(FEATURE_CORE);
                FOR_EACH_VK_FEATURE_1_3(FEATURE_CORE);
                FOR_EACH_VK_FEATURE_EXT(FEATURE_EXT);

#undef FEATURE_CORE
#undef FEATURE_EXT
                VkPhysicalDeviceFeatures features;
        };
        Extensions extensions_{};
        Properties properties_;
        Features features_{};
        VkPhysicalDeviceFeatures2 features2_{};
        VkPhysicalDeviceProperties2 properties2_{};
};

}  // namespace render::vulkan