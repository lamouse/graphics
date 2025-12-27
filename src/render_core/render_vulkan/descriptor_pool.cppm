module;
#include "common/class_traits.hpp"
#include <shared_mutex>
#include <vulkan/vulkan.hpp>
#include <memory>
export module render.vulkan.descriptor_pool;
import render.vulkan.common;
import render.vulkan.master_semaphore;
import render.vulkan.resource_pool;
import shader;
export namespace render::vulkan::resource {
class DescriptorAllocator;
struct DescriptorBankInfo;
struct DescriptorBank;

class DescriptorPool {
    public:
        explicit DescriptorPool(const Device& device, semaphore::MasterSemaphore& master_semaphore);
        ~DescriptorPool() = default;
        DescriptorPool() = delete;
        CLASS_NON_COPYABLE(DescriptorPool);
        CLASS_NON_MOVEABLE(DescriptorPool);
        auto allocator(vk::DescriptorSetLayout layout, std::span<const shader::Info> infos)
            -> DescriptorAllocator;
        auto allocator(vk::DescriptorSetLayout layout, const shader::Info& info)
            -> DescriptorAllocator;
        auto allocator(vk::DescriptorSetLayout layout, const DescriptorBankInfo& info)
            -> DescriptorAllocator;

    private:
        auto bank(const DescriptorBankInfo& reqs) -> DescriptorBank&;

        const Device& device_;
        semaphore::MasterSemaphore& master_semaphore_;

        std::shared_mutex banks_mutex_;
        std::vector<DescriptorBankInfo> bank_infos_;
        std::vector<std::unique_ptr<DescriptorBank>> banks_;
};

struct DescriptorBankInfo {
        [[nodiscard]] auto isSuperset(const DescriptorBankInfo& subset) const noexcept -> bool;

        uint32_t uniform_buffers_{};  ///< Number of uniform buffer descriptors
        uint32_t storage_buffers_{};  ///< Number of storage buffer descriptors
        uint32_t texture_buffers_{};  ///< Number of texture buffer descriptors
        uint32_t image_buffers_{};    ///< Number of image buffer descriptors
        uint32_t textures_{};         ///< Number of texture descriptors
        uint32_t images_{};           ///< Number of image descriptors
        std::int32_t score_{};        ///< Number of descriptors in total
};

struct DescriptorBank {
        DescriptorBankInfo info;
        std::vector<VulkanDescriptorPool> pools;
};
class DescriptorAllocator final : public ResourcePool {
        friend class DescriptorPool;

    public:
        explicit DescriptorAllocator() = default;
        ~DescriptorAllocator() override = default;

        auto operator=(DescriptorAllocator&&) noexcept -> DescriptorAllocator& = default;
        DescriptorAllocator(DescriptorAllocator&&) noexcept = default;
        CLASS_NON_COPYABLE(DescriptorAllocator);

        auto commit() -> vk::DescriptorSet;

    private:
        explicit DescriptorAllocator(const Device& device,
                                     semaphore::MasterSemaphore& master_semaphore,
                                     DescriptorBank& bank, vk::DescriptorSetLayout layout);

        void allocate(size_t begin, size_t end) override;

        auto allocateDescriptors(size_t count) -> DescriptorSets;

        const Device* device_{};
        DescriptorBank* bank_{};
        vk::DescriptorSetLayout layout_;

        std::vector<DescriptorSets> sets_;
};

}  // namespace render::vulkan::resource