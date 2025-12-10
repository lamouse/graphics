#include "resource_pool.hpp"
#include "master_semaphore.hpp"
namespace render::vulkan::resource {
ResourcePool::ResourcePool(semaphore::MasterSemaphore* master_semaphore, size_t grow_step)
    : master_semaphore_{master_semaphore}, grow_step_{grow_step} {}

auto ResourcePool::commitResource() -> size_t {
    // Refresh semaphore to query updated results
    master_semaphore_->refresh();
    const uint64_t gpu_tick = master_semaphore_->knownGpuTick();
    const auto search = [this, gpu_tick](size_t begin, size_t end) -> std::optional<size_t> {
        for (size_t iterator = begin; iterator < end; ++iterator) {
            if (gpu_tick >= ticks_[iterator]) {
                ticks_[iterator] = master_semaphore_->currentTick();
                return iterator;
            }
        }
        return std::nullopt;
    };
    // Try to find a free resource from the hinted position to the end.
    std::optional<size_t> found = search(hint_iterator_, ticks_.size());
    if (!found) {
        // Search from beginning to the hinted position.
        found = search(0, hint_iterator_);
        if (!found) {
            // Both searches failed, the pool is full; handle it.
            const size_t free_resource = manageOverflow();

            ticks_[free_resource] = master_semaphore_->currentTick();
            found = free_resource;
        }
    }
    // Free iterator is hinted to the resource after the one that's been committed.
    hint_iterator_ = (*found + 1) % ticks_.size();
    return *found;
}

auto ResourcePool::manageOverflow() -> size_t {
    const size_t old_capacity = ticks_.size();
    grow();

    // The last entry is guaranteed to be free, since it's the first element of the freshly
    // allocated resources.
    return old_capacity;
}

void ResourcePool::grow() {
    const size_t old_capacity = ticks_.size();
    ticks_.resize(old_capacity + grow_step_);
    allocate(old_capacity, old_capacity + grow_step_);
}
}  // namespace render::vulkan::resource