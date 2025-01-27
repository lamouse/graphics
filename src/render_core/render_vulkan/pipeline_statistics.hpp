#pragma once
#include <mutex>
#include <vector>
#include <vulkan/vulkan.hpp>
#include "common/common_types.hpp"
/**
 * @brief 用于vulkan统计pipeline的性能
 *
 */
namespace render::vulkan {
class Device;
namespace pipeline {

class PipelineStatistics {
    public:
        explicit PipelineStatistics(const Device& device_);

        void Collect(vk::Pipeline pipeline);

        void Report() const;

    private:
        struct Stats {
                u64 code_size{};
                u64 register_count{};
                u64 sgpr_count{};
                u64 vgpr_count{};
                u64 branches_count{};
                u64 basic_block_count{};
        };

        const Device& device;
        mutable std::mutex mutex;
        std::vector<Stats> collected_stats;
};
}  // namespace pipeline

}  // namespace render::vulkan