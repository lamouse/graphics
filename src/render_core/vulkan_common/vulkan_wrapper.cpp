#include "vulkan_wrapper.hpp"
#include "common/common_types.hpp"
#include <vulkan/vk_enum_string_helper.h>
#include "vulkan_common.hpp"
namespace render::vulkan {
namespace utils {
auto VulkanException::what() const noexcept -> const char* {
    return string_VkResult(static_cast<VkResult>(result));
}

}  // namespace utils
namespace {
template <typename Func>
void SortPhysicalDevices(std::vector<vk::PhysicalDevice>& devices, Func&& func) {
    // Calling GetProperties calls Vulkan more than needed. But they are supposed to be cheap
    // functions.
    std::stable_sort(devices.begin(), devices.end(),
                     [&func](vk::PhysicalDevice lhs, vk::PhysicalDevice rhs) {
                         return func(lhs.getProperties(), rhs.getProperties());
                     });
}

void SortPhysicalDevicesPerVendor(std::vector<vk::PhysicalDevice>& devices,
                                  std::initializer_list<u32> vendor_ids) {
    for (auto it = vendor_ids.end(); it != vendor_ids.begin();) {
        --it;
        SortPhysicalDevices(devices, [id = *it](const auto& lhs, const auto& rhs) {
            return lhs.vendorID == id && rhs.vendorID != id;
        });
    }
}

auto IsMicrosoftDozen(const char* device_name) -> bool {
    return std::strstr(device_name, "Microsoft") != nullptr;
}

void SortPhysicalDevices(std::vector<vk::PhysicalDevice>& devices) {
    // Sort by name, this will set a base and make GPUs with higher numbers appear first
    // (e.g. GTX 1650 will intentionally be listed before a GTX 1080).
    SortPhysicalDevices(devices, [](const auto& lhs, const auto& rhs) {
        return std::string_view{lhs.deviceName} > std::string_view{rhs.deviceName};
    });
    // Prefer discrete over non-discrete
    SortPhysicalDevices(devices, [](const auto& lhs, const auto& rhs) {
        return lhs.deviceType == vk::PhysicalDeviceType::eDiscreteGpu &&
               rhs.deviceType != vk::PhysicalDeviceType::eDiscreteGpu;
    });
    // Prefer Nvidia over AMD, AMD over Intel, Intel over the rest.
    SortPhysicalDevicesPerVendor(devices, {0x10DE, 0x1002, 0x8086});
    // Demote Microsoft's Dozen devices to the bottom.
    SortPhysicalDevices(
        devices, [](const VkPhysicalDeviceProperties& lhs, const VkPhysicalDeviceProperties& rhs) {
            return IsMicrosoftDozen(rhs.deviceName) && !IsMicrosoftDozen(lhs.deviceName);
        });
}

template <typename T>
void SetObjectName(vk::Device device, T handle, vk::ObjectType type, const char* name) {
    const vk::DebugUtilsObjectNameInfoEXT name_info{
        type,
        reinterpret_cast<u64>(&handle),
        name,
    };
    utils::check(device.setDebugUtilsObjectNameEXT(&name_info));
}
}  // namespace
void Image::SetObjectNameEXT(const char* name) const {
    SetObjectName(owner, handle, vk::ObjectType::eImage, name);
}

void Image::Release() const noexcept {
    if (handle) {
        vmaDestroyImage(allocator, handle, allocation);
    }
}

auto DeviceMemory::getMemoryFdKHR() const -> int {
    const vk::MemoryGetFdInfoKHR get_fd_info{handle};
    int fd;
    utils::check(owner.getMemoryFdKHR(&get_fd_info, &fd));
    return fd;
}

#ifdef _WIN32
auto DeviceMemory::getMemoryWin32HandleKHR() const -> HANDLE {
    const VkMemoryGetWin32HandleInfoKHR get_win32_handle_info{
        .sType = VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR,
        .pNext = nullptr,
        .memory = handle,
        .handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT_KHR,
    };
    HANDLE win32_handle;
    utils::check(static_cast<vk::Result>(
        vkGetMemoryWin32HandleKHR(owner, &get_win32_handle_info, &win32_handle)));
    return win32_handle;
}
#endif

void DeviceMemory::SetObjectNameEXT(const char* name) const {
    SetObjectName(owner, handle, vk::ObjectType::eDeviceMemory, name);
}

void Buffer::Flush() const {
    if (!is_coherent) {
        vmaFlushAllocation(allocator, allocation, 0, VK_WHOLE_SIZE);
    }
}

void Buffer::Invalidate() const {
    if (!is_coherent) {
        vmaInvalidateAllocation(allocator, allocation, 0, VK_WHOLE_SIZE);
    }
}

void Buffer::SetObjectNameEXT(const char* name) const {
    SetObjectName(owner, handle, vk::ObjectType::eBuffer, name);
}

void Buffer::Release() const noexcept {
    if (handle) {
        vmaDestroyBuffer(allocator, handle, allocation);
    }
}

Instance Instance::Create(u32 version, std::span<const char*> layers,
                          std::span<const char*> extensions) {
#ifdef __APPLE__
    constexpr vk::InstanceCreateFlags ci_flags{
        vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR};
#else
    constexpr vk::InstanceCreateFlags ci_flags{};
#endif

    const vk::ApplicationInfo application_info{

        "graphics engine",        VK_MAKE_VERSION(0, 1, 0), "graphics engine",
        VK_MAKE_VERSION(0, 1, 0), VK_API_VERSION_1_3,
    };
    vk::InstanceCreateInfo ci{ci_flags, &application_info, layers, extensions};
    vk::Instance instance = ::vk::createInstance(ci);

    return Instance(instance, wrapper::NoOwner{});
}

auto Instance::EnumeratePhysicalDevices() const -> std::vector<vk::PhysicalDevice> {
    u32 num;
    auto physical_devices = handle.enumeratePhysicalDevices();
    ;
    SortPhysicalDevices(physical_devices);
    return physical_devices;
}

auto Instance::CreateDebugUtilsMessenger(
    const vk::DebugUtilsMessengerCreateInfoEXT& create_info) const -> DebugUtilsMessenger {
    vk::DebugUtilsMessengerEXT object = handle.createDebugUtilsMessengerEXT(
        create_info, nullptr, vk::DispatchLoaderDynamic{handle, vkGetInstanceProcAddr});
    return DebugUtilsMessenger(object, handle);
}

DebugReportCallback Instance::CreateDebugReportCallback(
    const vk::DebugReportCallbackCreateInfoEXT& create_info) const {
    vk::DebugReportCallbackEXT object = handle.createDebugReportCallbackEXT(
        create_info, nullptr, vk::DispatchLoaderDynamic{handle, vkGetInstanceProcAddr});
    return DebugReportCallback(object, handle);
}

void BufferView::SetObjectNameEXT(const char* name) const {
    SetObjectName(owner, handle, vk::ObjectType::eBufferView, name);
}

void ImageView::SetObjectNameEXT(const char* name) const {
    SetObjectName(owner, handle, vk::ObjectType::eImageView, name);
}

}  // namespace render::vulkan