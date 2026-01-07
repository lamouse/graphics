module;
#include <vector>
#include <cstdint>
export module render.vulkan.resource_pool;
import render.vulkan.common;
import render.vulkan.master_semaphore;

export namespace render::vulkan::resource {
class ResourcePool {
    public:
        explicit ResourcePool() = default;
        explicit ResourcePool(semaphore::MasterSemaphore* master_semaphore, size_t grow_step);

        virtual ~ResourcePool() = default;
        ResourcePool(const ResourcePool&) = default;
        ResourcePool(ResourcePool&&) noexcept = default;
        auto operator=(const ResourcePool&) -> ResourcePool& = default;
        auto operator=(ResourcePool&&) noexcept -> ResourcePool& = default;


    protected:
        auto commitResource() -> size_t;

        /// Called when a chunk of resources have to be allocated.
        virtual void allocate(size_t begin, size_t end) = 0;

    private:
        /// Manages pool overflow allocating new resources.
        auto manageOverflow() -> size_t;

        /// Allocates a new page of resources.
        void grow();

        semaphore::MasterSemaphore* master_semaphore_{};
        size_t grow_step_ = 0;      ///< Number of new resources created after an overflow
        size_t hint_iterator_ = 0;  ///< Hint to where the next free resources is likely to be found
        std::vector<std::uint64_t> ticks_;  ///< Ticks for each resource
};
} // namespace render::vulkan::resource
