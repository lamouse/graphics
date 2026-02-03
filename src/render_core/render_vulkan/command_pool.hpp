#pragma once
#include "resource_pool.hpp"
#include <vulkan/vulkan.hpp>
namespace render::vulkan {
class Device;
namespace semaphore {
class MasterSemaphore;
}
namespace resource {

class CommandPool final : public ResourcePool {
    public:
        explicit CommandPool(semaphore::MasterSemaphore* master_semaphore_, const Device& device_);
        ~CommandPool() override;
        CommandPool(const CommandPool&) = delete;
        CommandPool(CommandPool&&) noexcept = delete;
        auto operator=(const CommandPool&) -> CommandPool& = delete;
        auto operator=(CommandPool&&) noexcept -> CommandPool& = delete;

        void allocate(size_t begin, size_t end) override;

        auto commit() -> vk::CommandBuffer;

    private:
        struct Pool;

        const Device& device_;
        std::vector<Pool> pools_;
};
}  // namespace resource
}  // namespace render::vulkan