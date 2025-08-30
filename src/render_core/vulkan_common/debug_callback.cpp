#include "debug_callback.hpp"
#include <spdlog/spdlog.h>
namespace render::vulkan {
namespace {
VKAPI_ATTR auto VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                         VkDebugUtilsMessageTypeFlagsEXT  /*messageType*/,
                                         const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                         void* /*pUserData*/) -> VkBool32 {
    switch (messageSeverity) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: {
            spdlog::warn("validation layer: {}", pCallbackData->pMessage);
            break;
        }
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: {
            spdlog::info("validation layer: {}", pCallbackData->pMessage);
            break;
        }
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: {
            spdlog::error("validation layer: {}", pCallbackData->pMessage);
            break;
        }
        default:
            spdlog::error("validation layer unknow messageSeverity: {}", pCallbackData->pMessage);
    }

    return VK_FALSE;
}
}  // namespace
auto createDebugMessenger(::vk::Instance instance) -> DebugUtilsMessenger {
    ::vk::DebugUtilsMessengerCreateInfoEXT createInfo =
        ::vk::DebugUtilsMessengerCreateInfoEXT()
            .setMessageSeverity(vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
                                vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                                // vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
                                vk::DebugUtilsMessageSeverityFlagBitsEXT::eError)
            .setMessageType(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                            vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                            vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance)
                            .setPfnUserCallback(debugCallback);
    return DebugUtilsMessenger{
        instance.createDebugUtilsMessengerEXT(
            createInfo, nullptr, vk::DispatchLoaderDynamic{instance, vkGetInstanceProcAddr}),
        instance};
}
}  // namespace render::vulkan