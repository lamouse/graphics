#include "command_pool.hpp"
#include <vulkan/vulkan.hpp>
#include "vulkan_common/vulkan_wrapper.hpp"
#include "vulkan_common/device.hpp"
#include "master_semaphore.hpp"

namespace render::vulkan::resource {
namespace {
constexpr size_t COMMAND_BUFFER_POOL_SIZE = 4;
}
struct CommandPool::Pool {
        vk::CommandPool handle_;
        CommandBuffers cmdbufs_;
};

CommandPool::CommandPool(semaphore::MasterSemaphore* master_semaphore_, const Device& device)
    : ResourcePool(master_semaphore_, COMMAND_BUFFER_POOL_SIZE), device_{device} {}

CommandPool::~CommandPool() = default;

void CommandPool::allocate(size_t begin, size_t end) {
    // Command buffers are going to be committed, recorded, executed every single usage cycle.
    // They are also going to be reset when committed.
    Pool& pool = pools_.emplace_back();
    ::vk::CommandPoolCreateInfo createInfo;
    createInfo.setQueueFamilyIndex(device_.getGraphicsFamily())
        .setFlags(::vk::CommandPoolCreateFlagBits::eResetCommandBuffer | ::vk::CommandPoolCreateFlagBits::eTransient);
    pool.handle_ = device_.getLogical().createCommandPool(createInfo);
    const ::vk::CommandBufferAllocateInfo allocInfo(pool.handle_, ::vk::CommandBufferLevel::ePrimary,
                                                    COMMAND_BUFFER_POOL_SIZE);
    pool.cmdbufs_ =
        CommandBuffers{device_.getLogical().allocateCommandBuffers(allocInfo), device_.getLogical(), pool.handle_};
}

auto CommandPool::commit() -> vk::CommandBuffer {
    const size_t index = commitResource();
    const auto pool_index = index / COMMAND_BUFFER_POOL_SIZE;
    const auto sub_index = index % COMMAND_BUFFER_POOL_SIZE;
    return pools_[pool_index].cmdbufs_[sub_index];
}

}  // namespace render::vulkan::resource