#pragma once
#include <vulkan/vulkan.hpp>
#include "common/common_funcs.hpp"
#include <vector>
#include <type_traits>
namespace render::vulkan::wrapper {
/// Array of a pool allocation.
/// Analogue to std::vector
template <typename AllocationType, typename PoolType>
class PoolAllocations {
    public:
        /// Construct an empty allocation.
        PoolAllocations() = default;
        CLASS_NON_COPYABLE(PoolAllocations);
        /// Construct an allocation. Errors are reported through IsOutOfPoolMemory().
        explicit PoolAllocations(std::vector<AllocationType> allocations_, vk::Device device_, PoolType pool) noexcept
            : allocations_{std::move(allocations_)}, num_{allocations_.size()}, device_{device_}, pool_{pool} {}
        explicit PoolAllocations(vk::Device device, PoolType pool, std::size_t num) noexcept
            : num_{num}, device_{device}, pool_{pool} {
            if constexpr (std::is_same_v<vk::CommandPool, PoolType> &&
                          std::is_same_v<vk::CommandBuffer, AllocationType>) {
                const ::vk::CommandBufferAllocateInfo allocInfo{pool_, ::vk::CommandBufferLevel::ePrimary, num_};
                allocations_ = device_.allocateCommandBuffers(allocInfo);
            }
        }
        explicit PoolAllocations(vk::Device device, std::size_t num, uint32_t graphics_family) noexcept
            : allocations_{std::move(allocations_)}, num_{num}, device_{device} {
            ::vk::CommandPoolCreateInfo createInfo;
            createInfo.setQueueFamilyIndex(graphics_family)
                .setFlags(::vk::CommandPoolCreateFlagBits::eResetCommandBuffer |
                          ::vk::CommandPoolCreateFlagBits::eTransient);
            pool_ = device.createCommandPool(createInfo);
            if constexpr (std::is_same_v<vk::CommandPool, PoolType> &&
                          std::is_same_v<vk::CommandBuffer, AllocationType>) {
                const ::vk::CommandBufferAllocateInfo allocInfo{pool_, ::vk::CommandBufferLevel::ePrimary, num_};
                allocations_ = device_.allocateCommandBuffers(allocInfo);
            }
        }
        /// Construct an allocation transferring ownership from another allocation.
        PoolAllocations(PoolAllocations&& rhs) noexcept
            : allocations_{std::move(rhs.allocations_)}, num_{rhs.num_}, device_{rhs.device_}, pool_{rhs.pool_} {
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

        /// Destructor to clean up resources.
        ~PoolAllocations() {
            if (device_ && pool_) {
                // Add code to clean up resources if necessary
            }
        }

        /// Returns the number of allocations.
        [[nodiscard]] auto size() const noexcept -> std::size_t { return num_; }

        /// Returns a pointer to the array of allocations.
        auto data() const noexcept -> AllocationType const* { return allocations_.get(); }

        /// Returns the allocation in the specified index.
        /// @pre index < size()
        auto operator[](std::size_t index) const noexcept -> AllocationType { return allocations_[index]; }

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
using CommandBuffers = wrapper::PoolAllocations<vk::CommandBuffer, vk::CommandPool>;
}