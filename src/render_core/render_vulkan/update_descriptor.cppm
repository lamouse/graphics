module;
#include <vulkan/vulkan.hpp>
export module render.vulkan:update_descriptor;
import render.vulkan.common;

export namespace render::vulkan {
namespace scheduler {
class Scheduler;
}

struct DescriptorUpdateEntry {
        struct Empty {};

        DescriptorUpdateEntry() = default;
        explicit DescriptorUpdateEntry(vk::DescriptorImageInfo image_) : image{image_} {}
        explicit DescriptorUpdateEntry(vk::DescriptorBufferInfo buffer_) : buffer{buffer_} {}
        explicit DescriptorUpdateEntry(vk::BufferView texel_buffer_)
            : texel_buffer{texel_buffer_} {}
        /// @cond
        union {
                Empty empty{};
                vk::DescriptorImageInfo image;
                vk::DescriptorBufferInfo buffer;
                vk::BufferView texel_buffer;
        };
        /// @endcond
};

class UpdateDescriptorQueue final {
        // This should be plenty for the vast majority of cases. Most desktop platforms only
        // provide up to 3 swapchain images.
        static constexpr size_t FRAMES_IN_FLIGHT = 8;
        static constexpr size_t FRAME_PAYLOAD_SIZE = 0x20000;
        static constexpr size_t PAYLOAD_SIZE = FRAME_PAYLOAD_SIZE * FRAMES_IN_FLIGHT;

    public:
        explicit UpdateDescriptorQueue(const Device& device_, scheduler::Scheduler& scheduler_);
        ~UpdateDescriptorQueue();
        void TickFrame();
        void Acquire();
        [[nodiscard]] auto UpdateData() const noexcept -> const DescriptorUpdateEntry* {
            return upload_start;
        }

        void AddSampledImage(vk::ImageView image_view, vk::Sampler sampler) {
            *(payload_cursor++) = DescriptorUpdateEntry(
                vk::DescriptorImageInfo{sampler, image_view, vk::ImageLayout::eGeneral});
        }

        void AddImage(vk::ImageView image_view) {
            *(payload_cursor++) = DescriptorUpdateEntry(
                vk::DescriptorImageInfo{VK_NULL_HANDLE, image_view, vk::ImageLayout::eGeneral});
        }

        void AddBuffer(vk::Buffer buffer, vk::DeviceSize offset, vk::DeviceSize size) {
            *(payload_cursor++) =
                DescriptorUpdateEntry(vk::DescriptorBufferInfo{buffer, offset, size});
        }

        void AddTexelBuffer(vk::BufferView texel_buffer) {
            *(payload_cursor++) = DescriptorUpdateEntry(texel_buffer);
        }

    private:
        const Device& device;
        scheduler::Scheduler& scheduler;

        size_t frame_index{0};
        DescriptorUpdateEntry* payload_cursor = nullptr;
        DescriptorUpdateEntry* payload_start = nullptr;
        const DescriptorUpdateEntry* upload_start = nullptr;
        std::array<DescriptorUpdateEntry, PAYLOAD_SIZE> payload;
};

using GuestDescriptorQueue = UpdateDescriptorQueue;
using ComputePassDescriptorQueue = UpdateDescriptorQueue;

}  // namespace render::vulkan