module;

#include <algorithm>
#include <span>
#include <mutex>
#include <shared_mutex>
#include <vulkan/vulkan.hpp>
module render.vulkan.descriptor_pool;
import render.vulkan.common;
import shader;
namespace render::vulkan::resource {
// Prefer small grow rates to avoid saturating the descriptor pool with barely used pipelines
constexpr size_t SETS_GROW_RATE = 16;
constexpr std::int32_t SCORE_THRESHOLD = 3;

auto DescriptorBankInfo::isSuperset(const DescriptorBankInfo& subset) const noexcept -> bool {
    return uniform_buffers_ >= subset.uniform_buffers_ &&
           storage_buffers_ >= subset.storage_buffers_ &&
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
        bank.uniform_buffers_ += accumulate(info.uniform_buffer_descriptors);
        bank.storage_buffers_ += accumulate(info.storage_buffers_descriptors);
        bank.texture_buffers_ += accumulate(info.texture_buffer_descriptors);
        bank.image_buffers_ += accumulate(info.image_buffer_descriptors);
        bank.textures_ += accumulate(info.texture_descriptors);
        bank.images_ += accumulate(info.image_descriptors);
    }
    bank.score_ = bank.uniform_buffers_ + bank.storage_buffers_ + bank.texture_buffers_ +
                  bank.image_buffers_ + bank.textures_ + bank.images_;
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
    vk::DescriptorPoolCreateInfo ci{vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind,
                                    sets_per_pool, static_cast<uint32_t>(pool_cursor),
                                    std::data(pool_sizes)};
    bank.pools.push_back(device.logical().createDescriptorPool(ci));
}

DescriptorAllocator::DescriptorAllocator(const Device& device,
                                         semaphore::MasterSemaphore& master_semaphore,
                                         DescriptorBank& bank, vk::DescriptorSetLayout layout)
    : ResourcePool(&master_semaphore, SETS_GROW_RATE),
      device_(&device),
      bank_(&bank),
      layout_(layout) {}

auto DescriptorAllocator::commit() -> vk::DescriptorSet {
    const size_t index = commitResource();
    return sets_[index / SETS_GROW_RATE][index % SETS_GROW_RATE];
}

void DescriptorAllocator::allocate(size_t begin, size_t end) {
    sets_.push_back(allocateDescriptors(end - begin));
}

auto DescriptorAllocator::allocateDescriptors(size_t count) -> DescriptorSets {
    const std::vector<vk::DescriptorSetLayout> layouts(count, layout_);
    vk::DescriptorSetAllocateInfo allocate_info{};
    allocate_info.setDescriptorPool(*bank_->pools.back()).setSetLayouts(layouts);

    return bank_->pools.back().Allocate(allocate_info);
}

DescriptorPool::DescriptorPool(const Device& device, semaphore::MasterSemaphore& master_semaphore)
    : device_{device}, master_semaphore_{master_semaphore} {}
auto DescriptorPool::allocator(vk::DescriptorSetLayout layout, std::span<const shader::Info> infos)
    -> DescriptorAllocator {
    return allocator(layout, makeBankInfo(infos));
}

auto DescriptorPool::allocator(vk::DescriptorSetLayout layout, const shader::Info& info)
    -> DescriptorAllocator {
    return allocator(layout, makeBankInfo(std::array{info}));
}

auto DescriptorPool::allocator(vk::DescriptorSetLayout layout, const DescriptorBankInfo& info)
    -> DescriptorAllocator {
    return DescriptorAllocator(device_, master_semaphore_, bank(info), layout);
}

auto DescriptorPool::bank(const DescriptorBankInfo& reqs) -> DescriptorBank& {
    std::shared_lock read_lock{banks_mutex_};
    const auto it = std::ranges::find_if(bank_infos_, [&reqs](DescriptorBankInfo& bank) {
        return std::abs(bank.score_ - reqs.score_) < SCORE_THRESHOLD && bank.isSuperset(reqs);
    });
    if (it != bank_infos_.end()) {
        return *banks_[std::distance(bank_infos_.begin(), it)];
    }
    read_lock.unlock();

    std::unique_lock write_lock{banks_mutex_};
    bank_infos_.push_back(reqs);

    auto& bank = *banks_.emplace_back(std::make_unique<DescriptorBank>());
    bank.info = reqs;
    allocatePool(device_, bank);
    return bank;
}

}  // namespace render::vulkan::resource