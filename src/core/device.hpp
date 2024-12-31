#pragma once
#include <cstdint>
#include <functional>
#include <optional>
#include <source_location>
#include <vulkan/vulkan.hpp>

#include "buffer.hpp"
#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif
namespace core {

/**
 * @brief Swapchain support
 *
 */
struct EXPORT SwapchainSupportDetails {
        ::vk::SurfaceCapabilitiesKHR capabilities;
        ::std::vector<::vk::SurfaceFormatKHR> formats;
        ::std::vector<::vk::PresentModeKHR> presentModes;
};

/**
 * @brief create device get surface
 *
 */
using CreateSurfaceFunc = ::std::function<::vk::SurfaceKHR(::vk::Instance)>;

/**
 * @brief vulkan device info add util
 *
 */
class EXPORT Device final {
    private:
    public:
        struct QueueFamilyIndices {
                std::optional<uint32_t> graphicsQueue;
                std::optional<uint32_t> computeQueue;
                std::optional<uint32_t> presentQueue;
                [[nodiscard]] auto isComplete() const {
                    return graphicsQueue.has_value() && presentQueue.has_value() &&
                           computeQueue.has_value();
                }
                [[nodiscard]] auto graphicsIndex() const {
                    if (!graphicsQueue.has_value()) {
                        throw std::runtime_error("no graphics queue");
                    }
                    return graphicsQueue.value();
                }
                [[nodiscard]] auto presentIndex() const {
                    if (!presentQueue.has_value()) {
                        throw std::runtime_error("no present queue");
                    }
                    return presentQueue.value();
                }
                [[nodiscard]] auto computeIndex() const {
                    if (!computeQueue.has_value()) {
                        throw std::runtime_error("no compute queue");
                    }
                    return computeQueue.value();
                }
        };

        enum class DeviceQueue : uint8_t { graphics, compute, present };
        explicit Device(const std::source_location& location = std::source_location::current());
        Device(const Device&) = default;
        Device(Device&&) = default;
        auto operator=(const Device&) -> Device& = default;
        auto operator=(Device&&) -> Device& = default;
        static void init(const std::vector<const char*>& instanceExtends,
                         const ::std::vector<const char*>& deviceExtensions,
                         const CreateSurfaceFunc& createFunc, bool enableValidationLayers = false);
        static void destroy();

        using CmdFunc = std::function<void(vk::CommandBuffer&)>;

        static auto getMaxMsaaSamples() -> ::vk::SampleCountFlagBits;
        auto findMemoryType(uint32_t typeFilter, ::vk::MemoryPropertyFlags properties) -> uint32_t;
        auto createBuffer(::vk::DeviceSize instance_size, uint32_t instance_count,
                          ::vk::BufferUsageFlags usage, ::vk::MemoryPropertyFlags properties)
            -> Buffer;
        void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, ::vk::Format format,
                         ::vk::SampleCountFlagBits numSamples, ::vk::ImageTiling tiling,
                         ::vk::ImageUsageFlags usage, ::vk::MemoryPropertyFlags properties,
                         ::vk::Image& image, ::vk::DeviceMemory& imageMemory);
        auto createImageView(::vk::Image image, ::vk::Format format,
                             ::vk::ImageAspectFlags aspectFlags, uint32_t mipLevels)
            -> ::vk::ImageView;
        auto getVKInstance() -> ::vk::Instance&;
        auto getSurface() -> ::vk::SurfaceKHR&;
        auto getPhysicalDevice() -> ::vk::PhysicalDevice&;
        auto getQueue(DeviceQueue queue) -> ::vk::Queue&;
        auto getQueueFamilyIndices() -> QueueFamilyIndices;
        /**
         * @brief vk logical device
         *
         * @return ::vk::Device&
         */
        auto logicalDevice() -> ::vk::Device&;
        auto findSupportedFormat(const std::vector<::vk::Format>& candidates,
                                 ::vk::ImageTiling tiling, ::vk::FormatFeatureFlags features)
            -> ::vk::Format;
        auto getCommandPool() -> ::vk::CommandPool&;
        auto querySwapchainSupport() -> SwapchainSupportDetails;
        void executeCmd(const CmdFunc& func) const;
        static void waitIdle();
        auto getMaxAnisotropy() -> float;
        ~Device();
};
}  // namespace core
