#include "descriptor_pool.hpp"
#include "vulkan_common/device.hpp"
namespace render::vulkan::resource {
// Prefer small grow rates to avoid saturating the descriptor pool with barely used pipelines
constexpr size_t SETS_GROW_RATE = 16;
constexpr std::int32_t SCORE_THRESHOLD = 3;
struct DescriptorBank {
        DescriptorBankInfo info;
        std::vector<vk::DescriptorPool> pools;
};
auto DescriptorBankInfo::isSuperset(const DescriptorBankInfo& subset) const noexcept -> bool {
    return uniform_buffers_ >= subset.uniform_buffers_ && storage_buffers_ >= subset.storage_buffers_ &&
           texture_buffers_ >= subset.texture_buffers_ && image_buffers_ >= subset.image_buffers_ &&
           textures_ >= subset.textures_ && images_ >= subset.image_buffers_;
}
template <typename Descriptors>
static auto accumulate(const Descriptors& descriptors) -> uint32_t {
    uint32_t count = 0;
    for (const auto& descriptor : descriptors) {
        count += descriptor.count;
    }
    return count;
}
static auto makeBankInfo(std::span<const shader::Info> infos) -> DescriptorBankInfo {
    DescriptorBankInfo bank;
    for (const shader::Info& info : infos) {
        bank.uniform_buffers_ += accumulate(info.constant_buffer_descriptors);
        bank.storage_buffers_ += accumulate(info.storage_buffers_descriptors);
        bank.texture_buffers_ += accumulate(info.texture_buffer_descriptors);
        bank.image_buffers_ += accumulate(info.image_buffer_descriptors);
        bank.textures_ += accumulate(info.texture_descriptors);
        bank.images_ += accumulate(info.image_descriptors);
    }
    bank.score_ = bank.uniform_buffers_ + bank.storage_buffers_ + bank.texture_buffers_ + bank.image_buffers_ +
                  bank.textures_ + bank.images_;
    return bank;
}
static void allocatePool(const Device& device, DescriptorBank& bank) {
    std::array<vk::DescriptorPoolSize, 6> pool_sizes;
    size_t pool_cursor{};
    const uint32_t sets_per_pool = device.getSetsPerPool();
    const auto add = [&](vk::DescriptorType type, uint32_t count) {
        if (count > 0) {
            pool_sizes[pool_cursor++] = {type, count * sets_per_pool};
        }
    };
    const auto& info{bank.info};
    add(vk::DescriptorType::eUniformBuffer, info.uniform_buffers_);
    add(vk::DescriptorType::eStorageBuffer, info.storage_buffers_);
    add(vk::DescriptorType::eUniformTexelBuffer, info.texture_buffers_);
    add(vk::DescriptorType::eStorageTexelBuffer, info.image_buffers_);
    add(vk::DescriptorType::eCombinedImageSampler, info.textures_);
    add(vk::DescriptorType::eStorageImage, info.images_);
    vk::DescriptorPoolCreateInfo ci{};
    ci.setPoolSizes(pool_sizes);
    bank.pools.push_back(device.getLogical().createDescriptorPool(ci));
}

DescriptorAllocator::DescriptorAllocator(const Device& device, semaphore::MasterSemaphore& master_semaphore,
                                         DescriptorBank& bank, VkDescriptorSetLayout layout)
    : ResourcePool(&master_semaphore, SETS_GROW_RATE), device_(&device), bank_(&bank), layout_(layout) {}

auto DescriptorAllocator::commit() -> vk::DescriptorSet {
    const size_t index = commitResource();
    return sets_[index / SETS_GROW_RATE][index % SETS_GROW_RATE];
}

}  // namespace render::vulkan::resource