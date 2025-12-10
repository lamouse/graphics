#pragma once
#include "resource_pool.hpp"
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

        void allocate(size_t begin, size_t end) override;

        auto commit() -> vk::CommandBuffer;

    private:
        struct Pool;

        const Device& device_;
        std::vector<Pool> pools_;
};
}  // namespace resource
}  // namespace render::vulkan