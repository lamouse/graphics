#pragma once
#include <vulkan/vulkan.hpp>
#include "common/common_funcs.hpp"
#include <vector>
#include <type_traits>
#include "common/common_types.hpp"
#include <span>
#include <utility>
#if defined(_WIN32)
#include <windows.h>
#endif
#ifdef _MSC_VER
#pragma warning(disable : 26812)  // Disable prefer enum class over enum
#endif
#undef max

VK_DEFINE_HANDLE(VmaAllocator)
VK_DEFINE_HANDLE(VmaAllocation)
namespace render::vulkan::wrapper {
/// Dummy type used to specify a handle has no owner.
struct NoOwner {
        NoOwner() = default;
        CLASS_DEFAULT_COPYABLE(NoOwner);
        CLASS_DEFAULT_MOVEABLE(NoOwner);
        ~NoOwner() = default;
        // NOLINTNEXTLINE
        NoOwner(std::nullptr_t) {}

        // auto operator=(std::nullptr_t) -> NoOwner { return {}; }  // NOLINT
        // auto operator=(NoOwner) -> NoOwner { return {}; }         // NOLINT
        operator bool() const { return false; }  // NOLINT
};
inline void destroy(vk::Device owner, vk::Fence handle) noexcept { owner.destroy(handle); }
inline void destroy(vk::Device owner, vk::Image handle) noexcept { owner.destroy(handle); }
inline void destroy(vk::Device owner, vk::DeviceMemory handle) noexcept { owner.free(handle); }
inline void destroy(vk::Device owner, vk::CommandPool handle) noexcept { owner.destroy(handle); }
inline void destroy(vk::Device owner, vk::ImageView handle) noexcept { owner.destroy(handle); }
inline void destroy(vk::Device owner, vk::BufferView handle) noexcept { owner.destroy(handle); }
inline void destroy(vk::Device owner, vk::Framebuffer handle) noexcept { owner.destroy(handle); }
inline void destroy(vk::Device owner, vk::Event handle) noexcept { owner.destroy(handle); }
inline void destroy(vk::Device owner, vk::SwapchainKHR handle) noexcept { owner.destroy(handle); }
inline void destroy(vk::Device owner, vk::ShaderModule handle) noexcept { owner.destroy(handle); }
inline void destroy(NoOwner /*unused*/, vk::Device handle) noexcept { handle.destroy(); }
inline void destroy(vk::Device owner, vk::Pipeline handle) noexcept { owner.destroy(handle); }
inline void destroy(vk::Device owner, vk::PipelineLayout handle) noexcept { owner.destroy(handle); }
inline void destroy(vk::Device owner, vk::PipelineCache handle) noexcept { owner.destroy(handle); }
inline void destroy(vk::Device owner, vk::RenderPass handle) noexcept { owner.destroy(handle); }
inline void destroy(vk::Device owner, vk::Sampler handle) noexcept { owner.destroy(handle); }
inline void destroy(vk::Device owner, vk::Semaphore handle) noexcept { owner.destroy(handle); }
inline void destroy(vk::Device owner, vk::QueryPool handle) noexcept { owner.destroy(handle); }
inline void destroy(vk::Device owner, vk::DescriptorPool handle) noexcept { owner.destroy(handle); }
inline void destroy(vk::Device owner, vk::Buffer handle) noexcept { owner.destroy(handle); }

inline void destroy(vk::Device owner, vk::DescriptorSetLayout handle) noexcept {
    owner.destroy(handle);
}
inline void destroy(vk::Device owner, vk::DescriptorUpdateTemplate handle) noexcept {
    owner.destroy(handle);
}

inline void destroy(vk::Instance owner, vk::SurfaceKHR val) noexcept { owner.destroy(val); }
inline void destroy(NoOwner /*unused*/, vk::Instance val) noexcept { val.destroy(); }
inline void destroy(vk::Instance owner, ::vk::DebugUtilsMessengerEXT handle) noexcept {
    owner.destroyDebugUtilsMessengerEXT(handle, nullptr);
}
inline void destroy(vk::Instance owner, vk::DebugReportCallbackEXT handle) noexcept {
    owner.destroyDebugReportCallbackEXT(handle, nullptr);
}

template <typename Type, typename OwnerType>
class Handle {
    public:
        /// Construct a handle and hold it's ownership.
        explicit Handle(Type handle_, OwnerType owner_) noexcept : handle{handle_}, owner{owner_} {}

        /// Construct an empty handle.
        Handle() = default;

        /// Construct an empty handle.
        // NOLINTNEXTLINE
        Handle(std::nullptr_t) {}

        /// Copying Vulkan objects is not supported and will never be.
        Handle(const Handle&) = delete;
        auto operator=(const Handle&) -> Handle& = delete;

        /// Construct a handle transferring the ownership from another handle.
        Handle(Handle&& rhs) noexcept
            : handle{std::exchange(rhs.handle, nullptr)}, owner{rhs.owner} {}

        /// Assign the current handle transferring the ownership from another handle.
        /// Destroys any previously held object.
        auto operator=(Handle&& rhs) noexcept -> Handle& {
            Release();
            handle = std::exchange(rhs.handle, nullptr);
            owner = rhs.owner;
            return *this;
        }

        /// Destroys the current handle if it existed.
        ~Handle() noexcept { Release(); }

        /// Destroys any held object.
        void reset() noexcept {
            Release();
            handle = nullptr;
        }

        /// Returns the address of the held object.
        /// Intended for Vulkan structures that expect a pointer to an array.
        [[nodiscard]] auto address() const noexcept -> const Type* {
            return std::addressof(handle);
        }

        /// Returns the held Vulkan handle.
        auto operator*() const noexcept -> Type { return handle; }

        /// Returns true when there's a held object.
        explicit operator bool() const noexcept { return handle != nullptr; }

    protected:
        Type handle = nullptr;
        OwnerType owner = nullptr;

    private:
        /// Destroys the held object if it exists.
        void Release() noexcept {
            if (handle) {
                destroy(owner, handle);
            }
        }
};

/// Array of a pool allocation.
/// Analogue to std::vector
template <typename AllocationType, typename PoolType>
class PoolAllocations {
    public:
        /// Construct an empty allocation.
        PoolAllocations() = default;
        ~PoolAllocations() = default;
        CLASS_NON_COPYABLE(PoolAllocations);
        /// Construct an allocation. Errors are reported through IsOutOfPoolMemory().
        explicit PoolAllocations(std::vector<AllocationType> allocations_, vk::Device device_,
                                 PoolType pool) noexcept
            : allocations_{std::move(allocations_)},
              num_{allocations_.size()},
              device_{device_},
              pool_{pool} {}
        explicit PoolAllocations(vk::Device device, PoolType pool, std::size_t num) noexcept
            : num_{num}, device_{device}, pool_{pool} {
            if constexpr (std::is_same_v<vk::CommandPool, PoolType> &&
                          std::is_same_v<vk::CommandBuffer, AllocationType>) {
                const ::vk::CommandBufferAllocateInfo allocInfo{
                    pool_, ::vk::CommandBufferLevel::ePrimary, num_};
                allocations_ = device_.allocateCommandBuffers(allocInfo);
            }
        }
        explicit PoolAllocations(vk::Device device, std::size_t num,
                                 uint32_t graphics_family) noexcept
            : allocations_{std::move(allocations_)}, num_{num}, device_{device} {
            ::vk::CommandPoolCreateInfo createInfo;
            createInfo.setQueueFamilyIndex(graphics_family)
                .setFlags(::vk::CommandPoolCreateFlagBits::eResetCommandBuffer |
                          ::vk::CommandPoolCreateFlagBits::eTransient);
            pool_ = device.createCommandPool(createInfo);
            if constexpr (std::is_same_v<vk::CommandPool, PoolType> &&
                          std::is_same_v<vk::CommandBuffer, AllocationType>) {
                const ::vk::CommandBufferAllocateInfo allocInfo{
                    pool_, ::vk::CommandBufferLevel::ePrimary, num_};
                allocations_ = device_.allocateCommandBuffers(allocInfo);
            }
        }
        /// Construct an allocation transferring ownership from another allocation.
        PoolAllocations(PoolAllocations&& rhs) noexcept
            : allocations_{std::move(rhs.allocations_)},
              num_{rhs.num_},
              device_{rhs.device_},
              pool_{rhs.pool_} {
            rhs.device_ = nullptr;
            rhs.pool_ = nullptr;
        }

        /// Assign an allocation transferring ownership from another allocation.
        auto operator=(PoolAllocations&& rhs) noexcept -> PoolAllocations& {
            allocations_ = std::move(rhs.allocations_);
            num_ = rhs.num_;
            device_ = rhs.device_;
            pool_ = rhs.pool_;
            rhs.device_ = nullptr;
            rhs.pool_ = nullptr;
            return *this;
        }

        /// Returns the number of allocations.
        [[nodiscard]] auto size() const noexcept -> std::size_t { return num_; }

        /// Returns a pointer to the array of allocations.
        auto data() const noexcept -> AllocationType const* { return allocations_.data(); }

        /// Returns the allocation in the specified index.
        /// @pre index < size()
        auto operator[](std::size_t index) const noexcept -> AllocationType {
            return allocations_[index];
        }

        /// True when a pool fails to construct.
        [[nodiscard]] auto isOutOfPoolMemory() const noexcept -> bool { return !device_; }

    private:
        std::vector<AllocationType> allocations_;
        std::size_t num_ = 0;
        vk::Device device_ = nullptr;
        PoolType pool_ = nullptr;
};

}  // namespace render::vulkan::wrapper

namespace render::vulkan {

namespace utils {
class VulkanException final : public std::exception {
    public:
        /// Construct the exception with a result.
        /// @pre result != VK_SUCCESS
        CLASS_DEFAULT_COPYABLE(VulkanException);
        CLASS_DEFAULT_MOVEABLE(VulkanException);
        explicit VulkanException(vk::Result result_) : result{result_} {}
        explicit VulkanException(VkResult result_) : result{static_cast<vk::Result>(result_)} {}
        ~VulkanException() override = default;
        /// @cond
        [[nodiscard]] auto what() const noexcept -> const char* override;
        /// @endcond
        [[nodiscard]] auto getResult() const noexcept -> vk::Result { return result; }

    private:
        vk::Result result;
};

/// Throws a Vulkan exception if result is not success.
inline void check(vk::Result result) {
    if (result != vk::Result::eSuccess) {
        throw VulkanException(result);
    }
}
inline void check(VkResult result) {
    if (result != VK_SUCCESS) {
        throw VulkanException(static_cast<vk::Result>(result));
    }
}
}  // namespace utils
using CommandBuffers = wrapper::PoolAllocations<vk::CommandBuffer, vk::CommandPool>;
using DescriptorSets = wrapper::PoolAllocations<vk::DescriptorSet, vk::DescriptorPool>;

using DebugUtilsMessenger = wrapper::Handle<vk::DebugUtilsMessengerEXT, vk::Instance>;
using DebugReportCallback = wrapper::Handle<vk::DebugReportCallbackEXT, vk::Instance>;
using SurfaceKHR = wrapper::Handle<vk::SurfaceKHR, vk::Instance>;
using DescriptorSetLayout = wrapper::Handle<vk::DescriptorSetLayout, vk::Device>;
using DescriptorUpdateTemplate = wrapper::Handle<vk::DescriptorUpdateTemplate, vk::Device>;
using Pipeline = wrapper::Handle<vk::Pipeline, vk::Device>;
using PipelineLayout = wrapper::Handle<vk::PipelineLayout, vk::Device>;
using QueryPool = wrapper::Handle<vk::QueryPool, vk::Device>;
using RenderPass = wrapper::Handle<vk::RenderPass, vk::Device>;
using Sampler = wrapper::Handle<vk::Sampler, vk::Device>;
class Instance : public wrapper::Handle<vk::Instance, wrapper::NoOwner> {
        using Handle<vk::Instance, wrapper::NoOwner>::Handle;

    public:
        /// Creates a Vulkan instance.
        /// @throw Exception on initialization error.
        static auto Create(u32 version, std::span<const char*> layers,
                           std::span<const char*> extensions) -> Instance;

        /// Enumerates physical devices.
        /// @return Physical devices and an empty handle on failure.
        /// @throw Exception on Vulkan error.
        [[nodiscard]] auto EnumeratePhysicalDevices() const -> std::vector<vk::PhysicalDevice>;

        /// Creates a debug callback messenger.
        /// @throw Exception on creation failure.
        [[nodiscard]] auto CreateDebugUtilsMessenger(
            const vk::DebugUtilsMessengerCreateInfoEXT& create_info) const -> DebugUtilsMessenger;

        /// Creates a debug report callback.
        /// @throw Exception on creation failure.
        [[nodiscard]] auto CreateDebugReportCallback(
            const vk::DebugReportCallbackCreateInfoEXT& create_info) const -> DebugReportCallback;
};

class Image {
    public:
        explicit Image(vk::Image handle_, vk::ImageUsageFlags usage_, vk::Device owner_,
                       VmaAllocator allocator_, VmaAllocation allocation_) noexcept
            : handle{handle_},
              usage{usage_},
              owner{owner_},
              allocator{allocator_},
              allocation{allocation_} {}
        Image() = default;

        CLASS_NON_COPYABLE(Image);

        Image(Image&& rhs) noexcept
            : handle{std::exchange(rhs.handle, nullptr)},
              usage{rhs.usage},
              owner{rhs.owner},
              allocator{rhs.allocator},
              allocation{rhs.allocation} {}

        auto operator=(Image&& rhs) noexcept -> Image& {
            Release();
            handle = std::exchange(rhs.handle, nullptr);
            usage = rhs.usage;
            owner = rhs.owner;
            allocator = rhs.allocator;
            allocation = rhs.allocation;
            return *this;
        }

        ~Image() noexcept { Release(); }

        auto operator*() const noexcept -> vk::Image { return handle; }

        void reset() noexcept {
            Release();
            handle = nullptr;
        }

        explicit operator bool() const noexcept { return handle != nullptr; }

        void SetObjectNameEXT(const char* name) const;

        [[nodiscard]] auto usageFlags() const noexcept -> vk::ImageUsageFlags { return usage; }

    private:
        void Release() const noexcept;

        vk::Image handle = nullptr;
        vk::ImageUsageFlags usage;
        vk::Device owner = nullptr;
        VmaAllocator allocator = nullptr;
        VmaAllocation allocation = nullptr;
};
class DeviceMemory : public wrapper::Handle<vk::DeviceMemory, vk::Device> {
        using Handle<vk::DeviceMemory, vk::Device>::Handle;

    public:
        [[nodiscard]] auto getMemoryFdKHR() const -> int;

#ifdef _WIN32
        [[nodiscard]] auto getMemoryWin32HandleKHR() const -> HANDLE;
#endif

        /// Set object name.
        void SetObjectNameEXT(const char* name) const;

        [[nodiscard]] auto map(vk::DeviceSize offset, vk::DeviceSize size) const -> u8* {
            void* data = owner.mapMemory(handle, offset, size);
            return static_cast<u8*>(data);
        }

        void unmap() const noexcept { owner.unmapMemory(handle); }
};

class Buffer {
    public:
        explicit Buffer(vk::Buffer handle_, vk::Device owner_, VmaAllocator allocator_,
                        VmaAllocation allocation_, std::span<u8> mapped_,
                        bool is_coherent_) noexcept
            : handle{handle_},
              owner{owner_},
              allocator{allocator_},
              allocation{allocation_},
              mapped{mapped_},
              is_coherent{is_coherent_} {}
        Buffer() = default;

        CLASS_NON_COPYABLE(Buffer);

        Buffer(Buffer&& rhs) noexcept
            : handle{std::exchange(rhs.handle, nullptr)},
              owner{rhs.owner},
              allocator{rhs.allocator},
              allocation{rhs.allocation},
              mapped{rhs.mapped},
              is_coherent{rhs.is_coherent} {}

        auto operator=(Buffer&& rhs) noexcept -> Buffer& {
            Release();
            handle = std::exchange(rhs.handle, nullptr);
            owner = rhs.owner;
            allocator = rhs.allocator;
            allocation = rhs.allocation;
            mapped = rhs.mapped;
            is_coherent = rhs.is_coherent;
            return *this;
        }

        ~Buffer() noexcept { Release(); }

        auto operator*() const noexcept -> vk::Buffer { return handle; }

        void reset() noexcept {
            Release();
            handle = nullptr;
        }

        explicit operator bool() const noexcept { return handle != nullptr; }

        /// Returns the host mapped memory, an empty span otherwise.
        auto Mapped() noexcept -> std::span<u8> { return mapped; }

        [[nodiscard]] auto Mapped() const noexcept -> std::span<const u8> { return mapped; }

        /// Returns true if the buffer is mapped to the host.
        [[nodiscard]] auto IsHostVisible() const noexcept -> bool { return !mapped.empty(); }

        void Flush() const;

        void Invalidate() const;

        void SetObjectNameEXT(const char* name) const;

    private:
        void Release() const noexcept;

        VkBuffer handle = nullptr;
        VkDevice owner = nullptr;
        VmaAllocator allocator = nullptr;
        VmaAllocation allocation = nullptr;
        std::span<u8> mapped;
        bool is_coherent = false;
};

class Queue {
    public:
        /// Construct an empty queue handle.
        constexpr Queue() noexcept = default;

        /// Construct a queue handle.
        constexpr explicit Queue(vk::Queue queue_) noexcept : queue{queue_} {}

        void Submit(std::span<vk::SubmitInfo> submit_infos,
                    vk::Fence fence = VK_NULL_HANDLE) const {
            queue.submit(submit_infos, fence);
        }

        [[nodiscard]] auto Present(const vk::PresentInfoKHR& present_info) const noexcept
            -> vk::Result {
            return queue.presentKHR(present_info);
        }

    private:
        vk::Queue queue = nullptr;
};

class VulkanBufferView : public wrapper::Handle<vk::BufferView, vk::Device> {
        using Handle<vk::BufferView, vk::Device>::Handle;

    public:
        /// Set object name.
        void SetObjectNameEXT(const char* name) const;
};
class ImageView : public wrapper::Handle<vk::ImageView, vk::Device> {
        using Handle<vk::ImageView, vk::Device>::Handle;

    public:
        /// Set object name.
        void SetObjectNameEXT(const char* name) const;
};

class Fence : public wrapper::Handle<vk::Fence, vk::Device> {
        using Handle<vk::Fence, vk::Device>::Handle;

    public:
        /// Set object name.
        void SetObjectNameEXT(const char* name) const;

        [[nodiscard]] auto Wait(u64 timeout = std::numeric_limits<u64>::max()) const noexcept
            -> vk::Result {
            return owner.waitForFences(handle, true, timeout);
        }

        [[nodiscard]] auto GetStatus() const noexcept -> vk::Result {
            return owner.getFenceStatus(handle);
        }

        void Reset() const { owner.resetFences(handle); }
};

class VulkanFramebuffer : public wrapper::Handle<vk::Framebuffer, vk::Device> {
        using Handle<vk::Framebuffer, vk::Device>::Handle;

    public:
        /// Set object name.
        void SetObjectNameEXT(const char* name) const;
};

class VulkanDescriptorPool : public wrapper::Handle<vk::DescriptorPool, vk::Device> {
        using Handle<vk::DescriptorPool, vk::Device>::Handle;

    public:
        [[nodiscard]] auto Allocate(const vk::DescriptorSetAllocateInfo& ai) const
            -> DescriptorSets;

        /// Set object name.
        void SetObjectNameEXT(const char* name) const;
};

class VulkanCommandPool : public wrapper::Handle<vk::CommandPool, vk::Device> {
        using Handle<vk::CommandPool, vk::Device>::Handle;

    public:
        [[nodiscard]] auto Allocate(std::size_t num_buffers,
                                    vk::CommandBufferLevel level =
                                        vk::CommandBufferLevel::ePrimary) const -> CommandBuffers;

        /// Set object name.
        void SetObjectNameEXT(const char* name) const;
};

class SwapchainKHR : public wrapper::Handle<vk::SwapchainKHR, vk::Device> {
        using Handle<vk::SwapchainKHR, vk::Device>::Handle;

    public:
        [[nodiscard]] auto GetImages() const -> std::vector<vk::Image>;
};

class Event : public wrapper::Handle<vk::Event, vk::Device> {
        using Handle<vk::Event, vk::Device>::Handle;

    public:
        /// Set object name.
        void SetObjectNameEXT(const char* name) const;

        auto GetStatus() const noexcept -> vk::Result { return owner.getEventStatus(handle); }
};

class ShaderModule : public wrapper::Handle<vk::ShaderModule, vk::Device> {
        using Handle<vk::ShaderModule, vk::Device>::Handle;

    public:
        /// Set object name.
        void SetObjectNameEXT(const char* name) const;
};

class VulkanPipelineCache : public wrapper::Handle<vk::PipelineCache, vk::Device> {
        using Handle<vk::PipelineCache, vk::Device>::Handle;

    public:
        /// Set object name.
        void SetObjectNameEXT(const char* name) const;

        auto Read(size_t* size, void* data) const noexcept -> vk::Result {
            return owner.getPipelineCacheData(handle, size, data);
        }
};

class Semaphore : public wrapper::Handle<vk::Semaphore, vk::Device> {
        using Handle<vk::Semaphore, vk::Device>::Handle;

    public:
        /// Set object name.
        void SetObjectNameEXT(const char* name) const;

        [[nodiscard]] auto GetCounter() const -> u64 {
            return owner.getSemaphoreCounterValue(handle);
        }

        /**
         * Waits for a timeline semaphore on the host.
         *
         * @param value   Value to wait
         * @param timeout Time in nanoseconds to timeout
         * @return        True on successful wait, false on timeout
         */
        [[nodiscard]] auto Wait(u64 value, u64 timeout = std::numeric_limits<u64>::max()) const
            -> bool {
            switch (auto result = owner.waitSemaphores(
                        vk::SemaphoreWaitInfo().setSemaphores(handle).setValues(value), timeout)) {
                case vk::Result::eSuccess:
                    return true;
                case vk::Result::eTimeout:
                    return false;
                default:
                    throw utils::VulkanException(result);
            }
        }
};

class LogicDevice : public wrapper::Handle<vk::Device, wrapper::NoOwner> {
        using Handle<vk::Device, wrapper::NoOwner>::Handle;

    public:
        static auto Create(vk::PhysicalDevice physical_device,
                           const std::vector<vk::DeviceQueueCreateInfo>& queues_ci,
                           const std::vector<const char*>& enabled_extensions, const void* next)
            -> LogicDevice;
        [[nodiscard]] auto createPipelineLayout(const vk::PipelineLayoutCreateInfo& ci) const
            -> PipelineLayout;

        [[nodiscard]] auto createPipeline(const vk::GraphicsPipelineCreateInfo& ci,
                                          const vk::PipelineCache& cache = {}) const -> Pipeline;

        [[nodiscard]] auto createPipeline(const vk::ComputePipelineCreateInfo& ci,
                                          const vk::PipelineCache& cache = {}) const -> Pipeline;
        [[nodiscard]] auto createPipelineCache(const vk::PipelineCacheCreateInfo& ci) const
            -> VulkanPipelineCache;
        [[nodiscard]] auto createFence(const vk::FenceCreateInfo& ci) const -> Fence;

        [[nodiscard]] auto createSemaphore() const -> Semaphore;
        Semaphore CreateSemaphore(const vk::SemaphoreCreateInfo& ci) const;

        [[nodiscard]] auto createDescriptorSetLayout(const vk::DescriptorSetLayoutCreateInfo&) const
            -> DescriptorSetLayout;

        [[nodiscard]] auto createShaderModel(const vk::ShaderModuleCreateInfo&) const
            -> ShaderModule;
        [[nodiscard]] auto createDescriptorUpdateTemplate(
            const vk::DescriptorUpdateTemplateCreateInfo&) const -> DescriptorUpdateTemplate;
        [[nodiscard]] auto createDescriptorPool(const vk::DescriptorPoolCreateInfo&) const
            -> VulkanDescriptorPool;

        [[nodiscard]] auto tryAllocateMemory(const VkMemoryAllocateInfo& ai) const noexcept
            -> DeviceMemory;

        [[nodiscard]] auto createCommandPool(const vk::CommandPoolCreateInfo& ci) const
            -> VulkanCommandPool;

        [[nodiscard]] auto createRenderPass(const vk::RenderPassCreateInfo&) const -> RenderPass;

        [[nodiscard]] auto CreateBufferView(const vk::BufferViewCreateInfo& ci) const
            -> VulkanBufferView;

        [[nodiscard]] auto CreateImageView(const vk::ImageViewCreateInfo& ci) const -> ImageView;
        [[nodiscard]] auto CreateSampler(const vk::SamplerCreateInfo& ci) const -> Sampler;

        [[nodiscard]] auto createFramerBuffer(const vk::FramebufferCreateInfo& ci) const
            -> VulkanFramebuffer;

        [[nodiscard]] auto createEvent(const vk::EventCreateInfo& ci) const -> Event;
        [[nodiscard]] auto createEvent() const -> Event;

        [[nodiscard]] auto createSwapchainKHR(const vk::SwapchainCreateInfoKHR& ci) const
            -> SwapchainKHR;

        void UpdateDescriptorSet(vk::DescriptorSet set,
                                 vk::DescriptorUpdateTemplate update_template,
                                 const void* data) const noexcept;

    private:
};

}  // namespace render::vulkan