#include "device_utils.hpp"
#include <spdlog/spdlog.h>
#include <vulkan/vk_enum_string_helper.h>
namespace render::vulkan::utils {
auto GetFormatProperties(vk::PhysicalDevice physical)
    -> std::unordered_map<vk::Format, vk::FormatProperties> {
    static constexpr std::array formats{
        vk::Format::eA1R5G5B5UnormPack16,
        vk::Format::eA2B10G10R10SintPack32,
        vk::Format::eA2B10G10R10SnormPack32,
        vk::Format::eA2B10G10R10SscaledPack32,
        vk::Format::eA2B10G10R10UintPack32,
        vk::Format::eA2B10G10R10UnormPack32,
        vk::Format::eA2B10G10R10UscaledPack32,
        vk::Format::eA2R10G10B10UnormPack32,
        vk::Format::eA8B8G8R8SintPack32,
        vk::Format::eA8B8G8R8SnormPack32,
        vk::Format::eA8B8G8R8SrgbPack32,
        vk::Format::eA8B8G8R8UintPack32,
        vk::Format::eA8B8G8R8UnormPack32,
        vk::Format::eAstc10x10SrgbBlock,
        vk::Format::eAstc10x10UnormBlock,
        vk::Format::eAstc10x5SrgbBlock,
        vk::Format::eAstc10x5UnormBlock,
        vk::Format::eAstc10x6SrgbBlock,
        vk::Format::eAstc10x6UnormBlock,
        vk::Format::eAstc10x8SrgbBlock,
        vk::Format::eAstc10x8UnormBlock,
        vk::Format::eAstc12x10SrgbBlock,
        vk::Format::eAstc12x10UnormBlock,
        vk::Format::eAstc12x12SrgbBlock,
        vk::Format::eAstc12x12UnormBlock,
        vk::Format::eAstc4x4SrgbBlock,
        vk::Format::eAstc4x4UnormBlock,
        vk::Format::eAstc5x4SrgbBlock,
        vk::Format::eAstc5x4UnormBlock,
        vk::Format::eAstc5x5SrgbBlock,
        vk::Format::eAstc5x5UnormBlock,
        vk::Format::eAstc6x5SrgbBlock,
        vk::Format::eAstc6x5UnormBlock,
        vk::Format::eAstc6x6SrgbBlock,
        vk::Format::eAstc6x6UnormBlock,
        vk::Format::eAstc8x5SrgbBlock,
        vk::Format::eAstc8x5UnormBlock,
        vk::Format::eAstc8x6SrgbBlock,
        vk::Format::eAstc8x6UnormBlock,
        vk::Format::eAstc8x8SrgbBlock,
        vk::Format::eAstc8x8UnormBlock,
        vk::Format::eB10G11R11UfloatPack32,
        vk::Format::eB4G4R4A4UnormPack16,
        vk::Format::eB5G5R5A1UnormPack16,
        vk::Format::eB5G6R5UnormPack16,
        vk::Format::eB8G8R8A8Srgb,
        vk::Format::eB8G8R8A8Unorm,
        vk::Format::eBc1RgbaSrgbBlock,
        vk::Format::eBc1RgbaUnormBlock,
        vk::Format::eBc2SrgbBlock,
        vk::Format::eBc2UnormBlock,
        vk::Format::eBc3SrgbBlock,
        vk::Format::eBc3UnormBlock,
        vk::Format::eBc4SnormBlock,
        vk::Format::eBc4UnormBlock,
        vk::Format::eBc5SnormBlock,
        vk::Format::eBc5UnormBlock,
        vk::Format::eBc6HSfloatBlock,
        vk::Format::eBc6HUfloatBlock,
        vk::Format::eBc7SrgbBlock,
        vk::Format::eBc7UnormBlock,
        vk::Format::eD16Unorm,
        vk::Format::eD16UnormS8Uint,
        vk::Format::eX8D24UnormPack32,
        vk::Format::eD24UnormS8Uint,
        vk::Format::eD32Sfloat,
        vk::Format::eD32SfloatS8Uint,
        vk::Format::eE5B9G9R9UfloatPack32,
        vk::Format::eR16G16B16A16Sfloat,
        vk::Format::eR16G16B16A16Sint,
        vk::Format::eR16G16B16A16Snorm,
        vk::Format::eR16G16B16A16Sscaled,
        vk::Format::eR16G16B16A16Uint,
        vk::Format::eR16G16B16A16Unorm,
        vk::Format::eR16G16B16A16Uscaled,
        vk::Format::eR16G16B16Sfloat,
        vk::Format::eR16G16B16Sint,
        vk::Format::eR16G16B16Snorm,
        vk::Format::eR16G16B16Sscaled,
        vk::Format::eR16G16B16Uint,
        vk::Format::eR16G16B16Unorm,
        vk::Format::eR16G16B16Uscaled,
        vk::Format::eR16G16Sfloat,
        vk::Format::eR16G16Sint,
        vk::Format::eR16G16Snorm,
        vk::Format::eR16G16Sscaled,
        vk::Format::eR16G16Uint,
        vk::Format::eR16G16Unorm,
        vk::Format::eR16G16Uscaled,
        vk::Format::eR16Sfloat,
        vk::Format::eR16Sint,
        vk::Format::eR16Snorm,
        vk::Format::eR16Sscaled,
        vk::Format::eR16Uint,
        vk::Format::eR16Unorm,
        vk::Format::eR16Uscaled,
        vk::Format::eR32G32B32A32Sfloat,
        vk::Format::eR32G32B32A32Sint,
        vk::Format::eR32G32B32A32Uint,
        vk::Format::eR32G32B32Sfloat,
        vk::Format::eR32G32B32Sint,
        vk::Format::eR32G32B32Uint,
        vk::Format::eR32G32Sfloat,
        vk::Format::eR32G32Sint,
        vk::Format::eR32G32Uint,
        vk::Format::eR32Sfloat,
        vk::Format::eR32Sint,
        vk::Format::eR32Uint,
        vk::Format::eR5G5B5A1UnormPack16,
        vk::Format::eR5G6B5UnormPack16,
        vk::Format::eR8G8B8A8Sint,
        vk::Format::eR8G8B8A8Snorm,
        vk::Format::eR8G8B8A8Srgb,
        vk::Format::eR8G8B8A8Sscaled,
        vk::Format::eR8G8B8A8Uint,
        vk::Format::eR8G8B8A8Unorm,
        vk::Format::eR8G8B8A8Uscaled,
        vk::Format::eR8G8B8Sint,
        vk::Format::eR8G8B8Snorm,
        vk::Format::eR8G8B8Sscaled,
        vk::Format::eR8G8B8Uint,
        vk::Format::eR8G8B8Unorm,
        vk::Format::eR8G8B8Uscaled,
        vk::Format::eR8G8Sint,
        vk::Format::eR8G8Snorm,
        vk::Format::eR8G8Sscaled,
        vk::Format::eR8G8Uint,
        vk::Format::eR8G8Unorm,
        vk::Format::eR8G8Uscaled,
        vk::Format::eR8Sint,
        vk::Format::eR8Snorm,
        vk::Format::eR8Sscaled,
        vk::Format::eR8Uint,
        vk::Format::eR8Unorm,
        vk::Format::eR8Uscaled,
        vk::Format::eS8Uint,
    };
    std::unordered_map<vk::Format, vk::FormatProperties> format_properties;
    for (const auto format : formats) {
        format_properties.emplace(format, physical.getFormatProperties(format));
    }
    return format_properties;
}

auto getNvidiaArchitecture(vk::PhysicalDevice physical,
                           const std::set<std::string, std::less<>>& exts) -> NvidiaArchitecture {
    if (exts.contains(VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME)) {
        VkPhysicalDeviceFragmentShadingRatePropertiesKHR shading_rate_props{};
        shading_rate_props.sType =
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_SHADING_RATE_PROPERTIES_KHR;
        VkPhysicalDeviceProperties2 physical_properties{};
        physical_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        physical_properties.pNext = &shading_rate_props;
        physical_properties = physical.getProperties2();
        if (shading_rate_props.primitiveFragmentShadingRateWithMultipleViewports) {
            // Only Ampere and newer support this feature
            // TODO: Find a way to differentiate Ampere and Ada
            return NvidiaArchitecture::Arch_AmpereOrNewer;
        }
        return NvidiaArchitecture::Arch_Turing;
    }

    if (exts.contains(VK_EXT_BLEND_OPERATION_ADVANCED_EXTENSION_NAME)) {
        VkPhysicalDeviceBlendOperationAdvancedPropertiesEXT advanced_blending_props{};
        advanced_blending_props.sType =
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BLEND_OPERATION_ADVANCED_PROPERTIES_EXT;
        VkPhysicalDeviceProperties2 physical_properties{};
        physical_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        physical_properties.pNext = &advanced_blending_props;
        physical_properties = physical.getProperties2();
        if (advanced_blending_props.advancedBlendMaxColorAttachments == 1) {
            return NvidiaArchitecture::Arch_Maxwell;
        }

        if (exts.contains(VK_EXT_CONSERVATIVE_RASTERIZATION_EXTENSION_NAME)) {
            VkPhysicalDeviceConservativeRasterizationPropertiesEXT conservative_raster_props{};
            conservative_raster_props.sType =
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_CONSERVATIVE_RASTERIZATION_PROPERTIES_EXT;
            physical_properties.pNext = &conservative_raster_props;
            physical_properties = physical.getProperties2();
            if (conservative_raster_props.degenerateLinesRasterized) {
                return NvidiaArchitecture::Arch_Volta;
            }
            return NvidiaArchitecture::Arch_Pascal;
        }
    }

    return NvidiaArchitecture::Arch_KeplerOrOlder;
}

auto checkBrokenCompute(vk::DriverId driver_id, uint32_t driver_version) -> bool {
    if (driver_id == vk::DriverId::eIntelProprietaryWindows) {
        const uint32_t major = VK_API_VERSION_MAJOR(driver_version);
        const uint32_t minor = VK_API_VERSION_MINOR(driver_version);
        const uint32_t patch = VK_API_VERSION_PATCH(driver_version);
        if (major == 0 && minor == 405 && patch < 286) {
            SPDLOG_WARN("Intel proprietary drivers 0.405.0 until 0.405.286 have broken compute");
            return true;
        }
    }
    return false;
}

auto extensionListForVulkan(const std::set<std::string, std::less<>>& extensions)
    -> std::vector<const char*> {
    std::vector<const char*> output;
    output.reserve(extensions.size());
    for (const auto& extension : extensions) {
        output.push_back(extension.c_str());
    }
    return output;
}
}  // namespace render::vulkan::utils