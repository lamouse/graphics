#include "debug_callback.hpp"
#include <spdlog/spdlog.h>
namespace render::vulkan {
namespace {

VKAPI_ATTR auto VKAPI_CALL
debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
              vk::DebugUtilsMessageTypeFlagsEXT /*messageType*/,
              const vk::DebugUtilsMessengerCallbackDataEXT* pCallbackData, void* /*pUserData*/)
    -> vk::Bool32 {
    switch (messageSeverity) {
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning: {
            spdlog::warn("validation layer: {}", pCallbackData->pMessage);
            break;
        }
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose:
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo: {
            spdlog::info("validation layer: {}", pCallbackData->pMessage);
            break;
        }
        case vk::DebugUtilsMessageSeverityFlagBitsEXT::eError: {
            spdlog::error("validation layer: {}", pCallbackData->pMessage);
            break;
        }
        default:
            spdlog::error("validation layer unknow messageSeverity: {}", pCallbackData->pMessage);
    }

    return vk::False;
}

}  // namespace
auto createDebugMessenger(::vk::Instance instance) -> DebugUtilsMessenger {
    ::vk::DebugUtilsMessengerCreateInfoEXT createInfo =
        ::vk::DebugUtilsMessengerCreateInfoEXT()
            .setMessageSeverity(vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
                                vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                                vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo |
                                vk::DebugUtilsMessageSeverityFlagBitsEXT::eError)
            .setMessageType(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                            vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                            vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance);
    createInfo.pfnUserCallback = debugCallback;
    return DebugUtilsMessenger{instance.createDebugUtilsMessengerEXT(createInfo, nullptr),
                               instance};
}
}  // namespace render::vulkan