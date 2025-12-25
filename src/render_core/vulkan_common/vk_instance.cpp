module;
#include <spdlog/spdlog.h>
#include <vector>
#include "vulkan_common.hpp"
#include "render_core/vulkan_common/vulkan_wrapper.hpp"
#include "core/frontend/window.hpp"

module render.vulkan.common.instance;

namespace render::vulkan {
namespace instance {
namespace {
constexpr const char* LOG_TAG = "Render_Vulkan: ";
[[nodiscard]] auto layers(bool enable_validation) -> std::vector<const char*> {
    std::vector<const char*> layers;
    if (enable_validation) {
        layers.push_back("VK_LAYER_KHRONOS_validation");
    }
    return layers;
}

void removeUnavailableLayers(std::vector<const char*>& layers) {
    auto layer_properties = vk::enumerateInstanceLayerProperties();

    if (layer_properties.empty()) {
        SPDLOG_ERROR("Failed to query layer properties, disabling layers");
        layers.clear();
    }
    std::erase_if(layers, [&layer_properties](const char* layer) {
        const auto comp = [layer](const VkLayerProperties& layer_property) {
            return std::strcmp(layer, layer_property.layerName) == 0;
        };
        const auto it = std::ranges::find_if(layer_properties, comp);
        if (it == layer_properties.end()) {
            SPDLOG_ERROR("Layer {} not available, removing it", layer);
            return true;
        }
        return false;
    });
}

[[nodiscard]] auto checkExtensionsSupported(std::span<const char* const> extensions) -> bool {
    auto properties = vk::enumerateInstanceExtensionProperties();
    if (properties.empty()) {
        SPDLOG_ERROR("{} Failed to query extension properties", LOG_TAG);
        return false;
    }
    for (const char* extension : extensions) {
        const auto it = std::ranges::find_if(properties, [extension](const auto& prop) {
            return std::strcmp(extension, prop.extensionName) == 0;
        });
        if (it == properties.end()) {
            SPDLOG_ERROR("{} Required instance extension {} is not available", LOG_TAG, extension);
            return false;
        }
    }
    return true;
}

[[nodiscard]] auto requiredExtensions(core::frontend::WindowSystemType wst, bool enable_validation)
    -> std::vector<const char*> {
    std::vector<const char*> extensions;
    switch (wst) {
        case core::frontend::WindowSystemType::Headless:
            break;
#ifdef _WIN32
        case core::frontend::WindowSystemType::Windows:
            extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
            break;
#elif defined(__APPLE__)
        case Core::Frontend::WindowSystemType::Cocoa:
            extensions.push_back(VK_EXT_METAL_SURFACE_EXTENSION_NAME);
            break;
#else
        case core::frontend::WindowSystemType::X11:
            extensions.push_back(VK_KHR_XLIB_SURFACE_EXTENSION_NAME);
            break;
        case core::frontend::WindowSystemType::Wayland:
            extensions.push_back(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
            break;
#endif
        default:
            SPDLOG_ERROR("{} Presentation not supported on this platform", LOG_TAG);
            break;
    }
    if (wst != core::frontend::WindowSystemType::Headless) {
        extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
    }
#ifdef __APPLE__
    if (checkExtensionsSupported(
            std::array{VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
                       VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME})) {
        extends.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
        extends.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
    }

#endif
    if (enable_validation &&
        checkExtensionsSupported(std::array{VK_EXT_DEBUG_UTILS_EXTENSION_NAME})) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    return extensions;
}
}  // namespace
}  // namespace instance
auto createInstance(uint32_t required_version, core::frontend::WindowSystemType wst,
                    bool enable_validation) -> Instance {
    VULKAN_HPP_DEFAULT_DISPATCHER.init();
    auto layers = instance::layers(enable_validation);
    instance::removeUnavailableLayers(layers);
    auto extensions = instance::requiredExtensions(wst, enable_validation);
    const auto available_version = vk::enumerateInstanceVersion();
    if (available_version < required_version) {
        SPDLOG_ERROR(
            "{} Required Vulkan version {}.{} is not available, available version is {}.{}",
            instance::LOG_TAG, VK_VERSION_MAJOR(required_version),
            VK_VERSION_MINOR(required_version), VK_VERSION_MAJOR(available_version),
            VK_VERSION_MINOR(available_version));
        throw std::runtime_error("Required Vulkan version is not available");
    }
    uint32_t version = VK_MAKE_VERSION(1, 3, 0);
    uint32_t app_version = VK_MAKE_VERSION(0, 1, 0);
    ::vk::ApplicationInfo appInfo{"graphics", app_version, "engine", app_version, version};
    ::vk::InstanceCreateInfo createInfo;
    createInfo
        .setPApplicationInfo(&appInfo)
#ifdef __APPLE__
        .setFlags(::vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR)
#endif
        .setPEnabledLayerNames(layers)
        .setPEnabledExtensionNames(extensions);
    return Instance::Create(version, layers, extensions);
}
}  // namespace render::vulkan