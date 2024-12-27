#include "system_config.hpp"
namespace config {
auto getDeviceExtensions() -> ::std::vector<const char *> {
    return ::std::vector<const char *>{
#if defined(VK_USE_PLATFORM_MACOS_MVK)
        "VK_KHR_portability_subset",  // "VK_KHR_portability_subset" macos
#endif
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        // VK_KHR_DYNAMIC_RENDERING_LOCAL_READ_EXTENSION_NAME
    };
}
}  // namespace config