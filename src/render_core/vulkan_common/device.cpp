#include "device.hpp"
#include <spdlog/spdlog.h>
namespace render::vulkan {
Device::Device(vk::Instance instance, vk::PhysicalDevice physical, vk::SurfaceKHR surface)
    : instance_(instance), physical_(physical) {
    format_properties_ = utils::GetFormatProperties(physical_);
    const bool is_suitable = getSuitability(surface != nullptr);
    const vk::DriverId driver_id = properties_.driver_.driverID;
    const auto device_id = properties_.properties_.deviceID;
    const bool is_radv = driver_id == ::vk::DriverId::eMesaRadv;
    const bool is_amd_driver =
        driver_id == ::vk::DriverId::eAmdProprietary || driver_id == ::vk::DriverId::eAmdOpenSource ;
const bool is_amd = is_amd_driver || is_radv;
    const bool is_intel_windows = driver_id == ::vk::DriverId::eIntelProprietaryWindows;
    const bool is_intel_anv = driver_id == ::vk::DriverId::eIntelOpenSourceMESA ;
    const bool is_nvidia = driver_id ==  ::vk::DriverId::eNvidiaProprietary;
    const bool is_mvk = driver_id == ::vk::DriverId::eMoltenvk;
    const bool is_qualcomm = driver_id == ::vk::DriverId::eQualcommProprietary;
    const bool is_turnip = driver_id == ::vk::DriverId::eMesaTurnip;
    const bool is_s8gen2 = device_id == 0x43050a01;
    const bool is_arm = driver_id == ::vk::DriverId::eArmProprietary;
        if ((is_mvk || is_qualcomm || is_turnip || is_arm) && !is_suitable) {
        SPDLOG_WARN("Unsuitable driver, continuing anyway");
    } else if (!is_suitable) {
        throw std::runtime_error("Unsuitable driver");
    }
    if (is_nvidia) {
        nvidia_arch = utils::getNvidiaArchitecture(physical, supported_extensions_);
    }
}
auto Device::getSuitability(bool requires_swapchain) -> bool {
    // Assume we will be suitable.
    bool suitable = true;

    return suitable;
}
}  // namespace render::vulkan
