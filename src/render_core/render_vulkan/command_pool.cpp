module;
#include <vulkan/vulkan.hpp>
module render.vulkan;
import render.vulkan.common;
import :resource_pool;
namespace render::vulkan::resource {
namespace {
constexpr size_t COMMAND_BUFFER_POOL_SIZE = 4;
}
struct CommandPool::Pool {
        VulkanCommandPool handle_;
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
        .setFlags(::vk::CommandPoolCreateFlagBits::eResetCommandBuffer |
                  ::vk::CommandPoolCreateFlagBits::eTransient);
    pool.handle_ = device_.logical().createCommandPool(createInfo);
    pool.cmdbufs_ = pool.handle_.Allocate(COMMAND_BUFFER_POOL_SIZE);
}

auto CommandPool::commit() -> vk::CommandBuffer {
    const size_t index = commitResource();
    const auto pool_index = index / COMMAND_BUFFER_POOL_SIZE;
    const auto sub_index = index % COMMAND_BUFFER_POOL_SIZE;
    return pools_[pool_index].cmdbufs_[sub_index];
}

}  // namespace render::vulkan::resource