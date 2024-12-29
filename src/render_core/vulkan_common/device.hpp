#pragma once
#include <vulkan/vulkan.hpp>
#include "device_utils.hpp"
#include <unordered_map>
#include <set>
/**
 * @brief vulkan device
 *
 */
namespace render::vulkan {
class Device {
    public:
        explicit Device(vk::Instance instance, vk::PhysicalDevice physical, vk::SurfaceKHR surface);
        ~Device();
        /// Returns the logical device.
        [[nodiscard]] auto getLogical() const -> const vk::Device& { return logical_; }

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

    private:
        vk::Instance instance_;
        vk::PhysicalDevice physical_;
        vk::Device logical_;
        vk::Queue graphics_queue_;
        vk::Queue present_queue_;
        vk::Queue compute_queue_;

        uint32_t graphics_family_;
        uint32_t present_family_;
        uint32_t compute_family_;
        uint32_t instance_version_{};
        /// Format properties dictionary.
        std::unordered_map<vk::Format, vk::FormatProperties> format_properties_;

        struct Properties {
                vk::PhysicalDeviceDriverProperties driver_;
                vk::PhysicalDeviceSubgroupProperties subgroup_properties_;
                vk::PhysicalDeviceFloatControlsProperties float_controls_;
                vk::PhysicalDevicePushDescriptorPropertiesKHR push_descriptor_;
                vk::PhysicalDeviceSubgroupSizeControlProperties subgroup_size_control_;
                vk::PhysicalDeviceTransformFeedbackPropertiesEXT transform_feedback_;

                vk::PhysicalDeviceProperties properties_;
        };

        Properties properties_;
        // Telemetry parameters
        std::set<std::string, std::less<>> supported_extensions_;  ///< Reported Vulkan extensions.
        std::set<std::string, std::less<>> loaded_extensions_;     ///< Loaded Vulkan extensions.
        auto GetSuitability(bool requires_swapchain) -> bool;

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
#define FEATURE_CORE(prefix, struct_name, macro_name, var_name) VkPhysicalDevice##struct_name##Features var_name{};
#define FEATURE_EXT(prefix, struct_name, macro_name, var_name) \
    VkPhysicalDevice##struct_name##Features##prefix var_name{};

                FOR_EACH_VK_FEATURE_1_1(FEATURE_CORE);
                FOR_EACH_VK_FEATURE_1_2(FEATURE_CORE);
                FOR_EACH_VK_FEATURE_1_3(FEATURE_CORE);
                FOR_EACH_VK_FEATURE_EXT(FEATURE_EXT);

#undef FEATURE_CORE
#undef FEATURE_EXT
                Extensions extensions_{};
                VkPhysicalDeviceFeatures features{};
                vk::PhysicalDeviceFeatures2 features2;
                vk::PhysicalDeviceProperties2 properties2;
        };
};

}  // namespace render::vulkan