module;
#include <vulkan/vulkan.hpp>
export module render.vulkan.command_pool;
import render.vulkan.common;
import render.vulkan.resource_pool;
import render.vulkan.master_semaphore;

export namespace render::vulkan::resource {

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
} // namespace render::vulkan::resource
