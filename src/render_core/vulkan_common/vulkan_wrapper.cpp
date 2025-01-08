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
    const VkDebugUtilsObjectNameInfoEXT name_info{
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .pNext = nullptr,
        .objectType = static_cast<VkObjectType>(type),
        .objectHandle = reinterpret_cast<u64>(&handle),
        .pObjectName = name,
    };
    auto fun = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetDeviceProcAddr(
        device, "vkSetDebugUtilsObjectNameEXT");
    if (fun) {
        utils::check(fun(device, &name_info));
    }
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
    auto fun = (PFN_vkGetMemoryFdKHR)vkGetDeviceProcAddr(owner, "vkGetMemoryFdKHR");
    const VkMemoryGetFdInfoKHR get_fd_info{
        .memory = handle, .handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT_KHR};
    int fd;
    if (!fun) {
        throw utils::VulkanException(VK_ERROR_EXTENSION_NOT_PRESENT);
    }
    utils::check(fun(owner, &get_fd_info, &fd));
    return fd;
}

#ifdef _WIN32
auto DeviceMemory::getMemoryWin32HandleKHR() const -> HANDLE {
    auto fun =
        (PFN_vkGetMemoryWin32HandleKHR)vkGetDeviceProcAddr(owner, "vkGetMemoryWin32HandleKHR");
    const VkMemoryGetWin32HandleInfoKHR get_win32_handle_info{
        .sType = VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR,
        .pNext = nullptr,
        .memory = handle,
        .handleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT_KHR,
    };
    HANDLE win32_handle;
    utils::check(static_cast<vk::Result>(fun(owner, &get_win32_handle_info, &win32_handle)));
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
void Framebuffer::SetObjectNameEXT(const char* name) const {
    SetObjectName(owner, handle, vk::ObjectType::eFramebuffer, name);
}

DescriptorSets VulkanDescriptorPool::Allocate(const vk::DescriptorSetAllocateInfo& ai) const {
    const std::size_t num = ai.descriptorSetCount;
    std::vector<::vk::DescriptorSet> sets(num);
    switch (auto result = owner.allocateDescriptorSets(&ai, sets.data())) {
        case vk::Result::eSuccess:
            return DescriptorSets(std::move(sets), owner, handle);
        case vk::Result::eErrorOutOfPoolMemory:
            return {};
        default:
            throw utils::VulkanException(result);
    }
}

void VulkanDescriptorPool::SetObjectNameEXT(const char* name) const {
    SetObjectName(owner, handle, vk::ObjectType::eDescriptorPool, name);
}

CommandBuffers CommandPool::Allocate(std::size_t num_buffers, vk::CommandBufferLevel level) const {
    const vk::CommandBufferAllocateInfo ai{handle, level, static_cast<u32>(num_buffers)};

    std::vector<vk::CommandBuffer> command_buffers(num_buffers);
    switch (const vk::Result result = owner.allocateCommandBuffers(&ai, command_buffers.data())) {
        case vk::Result::eSuccess:
            return CommandBuffers(std::move(command_buffers), owner, handle);
        case vk::Result::eErrorOutOfPoolMemory:
            return {};
        default:
            throw utils::VulkanException(result);
    }
}

void CommandPool::SetObjectNameEXT(const char* name) const {
    SetObjectName(owner, handle, vk::ObjectType::eCommandPool, name);
}

std::vector<vk::Image> SwapchainKHR::GetImages() const {
    return owner.getSwapchainImagesKHR(handle);
}

void Event::SetObjectNameEXT(const char* name) const {
    SetObjectName(owner, handle, vk::ObjectType::eEvent, name);
}

void ShaderModule::SetObjectNameEXT(const char* name) const {
    SetObjectName(owner, handle, vk::ObjectType::eShaderModule, name);
}

void PipelineCache::SetObjectNameEXT(const char* name) const {
    SetObjectName(owner, handle, vk::ObjectType::ePipelineCache, name);
}
void Semaphore::SetObjectNameEXT(const char* name) const {
    SetObjectName(owner, handle, vk::ObjectType::eSemaphore, name);
}

LogicDevice LogicDevice::Create(vk::PhysicalDevice physical_device,
                                const std::vector<vk::DeviceQueueCreateInfo>& queues_ci,
                                const std::vector<const char*>& enabled_extensions,
                                const void* next) {
    vk::DeviceCreateInfo ci{};
    ci.setQueueCreateInfos(queues_ci).setPEnabledExtensionNames(enabled_extensions).setPNext(next);
    return LogicDevice(physical_device.createDevice(ci), wrapper::NoOwner{});
}

auto LogicDevice::createPipelineLayout(this auto&& self, const vk::PipelineLayoutCreateInfo& ci)
    -> PipelineLayout {
    auto layout = self.handle.createPipelineLayout(ci);
    return PipelineLayout{layout, self.handle};
}

auto LogicDevice::createPipeline(this auto&& self, const vk::GraphicsPipelineCreateInfo& ci,
                                 const vk::PipelineCache& cache) -> Pipeline {
    auto result = self.handle.createGraphicsPipeline(cache, ci);
    utils::check(result.result);
    return Pipeline{result.value, self.handle};
}

auto LogicDevice::createPipeline(this auto&& self, const vk::ComputePipelineCreateInfo& ci,
                                 const vk::PipelineCache& cache) -> Pipeline {
    auto result = self.handle.createComputePipelines(cache, ci);
    utils::check(result.result);
    return Pipeline{result.value, self.handle};
}

auto LogicDevice::createFence(const vk::FenceCreateInfo& ci) const -> Fence {
    auto fence = handle.createFence(ci);
    return Fence{fence, handle};
}

auto LogicDevice::createSemaphore() const -> Semaphore {
    auto semaphore = handle.createSemaphore(vk::SemaphoreCreateInfo{});
    return Semaphore(semaphore, handle);
}

auto LogicDevice::createDescriptorSetLayout(const vk::DescriptorSetLayoutCreateInfo& ci) const
    -> DescriptorSetLayout {
    vk::DescriptorSetLayout layout = handle.createDescriptorSetLayout(ci, nullptr);
    return DescriptorSetLayout{layout, handle};
}

auto LogicDevice::createShaderModel(const vk::ShaderModuleCreateInfo& ci) const -> ShaderModule {
    auto shader = handle.createShaderModule(ci);
    return ShaderModule(shader, handle);
}

auto LogicDevice::createDescriptorUpdateTemplate(
    const vk::DescriptorUpdateTemplateCreateInfo& ci) const -> DescriptorUpdateTemplate {
    auto template_ = handle.createDescriptorUpdateTemplate(ci);
    return DescriptorUpdateTemplate{template_, handle};
}

auto LogicDevice::createDescriptorPool(const vk::DescriptorPoolCreateInfo& ci) const
    -> VulkanDescriptorPool {
    auto pool = handle.createDescriptorPool(ci);
    return VulkanDescriptorPool{pool, handle};
}

auto LogicDevice::tryAllocateMemory(const VkMemoryAllocateInfo& ai) const noexcept -> DeviceMemory {
    VkDeviceMemory memory = handle.allocateMemory(ai);
    return DeviceMemory(memory, handle);
}

[[nodiscard]] auto LogicDevice::createCommandPool(const vk::CommandPoolCreateInfo& ci) const
    -> CommandPool {
    return CommandPool{handle.createCommandPool(ci), handle};
}
[[nodiscard]] auto LogicDevice::createRenderPass(const vk::RenderPassCreateInfo& ci) const
    -> RenderPass {
    auto render_pass = handle.createRenderPass(ci);
    return RenderPass{render_pass, handle};
}

auto LogicDevice::CreateBufferView(const vk::BufferViewCreateInfo& ci) const -> BufferView {
    vk::BufferView object = handle.createBufferView(ci);
    return BufferView(object, handle);
}

auto LogicDevice::CreateImageView(const vk::ImageViewCreateInfo& ci) const -> ImageView {
    VkImageView object = handle.createImageView(ci);
    return ImageView(object, handle);
}

Semaphore LogicDevice::CreateSemaphore(const vk::SemaphoreCreateInfo& ci) const {
    VkSemaphore object = handle.createSemaphore(ci);
    return Semaphore(object, handle);
}

Sampler LogicDevice::CreateSampler(const vk::SamplerCreateInfo& ci) const {
    vk::Sampler object = handle.createSampler(ci);
    return Sampler(object, handle);
}

auto LogicDevice::createFramerBuffer(const vk::FramebufferCreateInfo& ci) const -> Framebuffer {
    auto frame_buffer = handle.createFramebuffer(ci);
    return Framebuffer{frame_buffer, handle};
}

}  // namespace render::vulkan