#include "update_descriptor.hpp"
#include "vulkan_common/device.hpp"
#include "scheduler.hpp"
#include <spdlog/spdlog.h>
namespace render::vulkan {
UpdateDescriptorQueue::UpdateDescriptorQueue(const Device& device_,
                                             scheduler::Scheduler& scheduler_)
    : device{device_}, scheduler{scheduler_} {
    payload_start = payload.data();
    payload_cursor = payload.data();
}

UpdateDescriptorQueue::~UpdateDescriptorQueue() = default;

void UpdateDescriptorQueue::TickFrame() {
    if (++frame_index >= FRAMES_IN_FLIGHT) {
        frame_index = 0;
    }
    payload_start = payload.data() + frame_index * FRAME_PAYLOAD_SIZE;
    payload_cursor = payload_start;
}

void UpdateDescriptorQueue::Acquire() {
    // Minimum number of entries required.
    // This is the maximum number of entries a single draw call might use.
    static constexpr size_t MIN_ENTRIES = 0x400;

    if (std::distance(payload_start, payload_cursor) + MIN_ENTRIES >= FRAME_PAYLOAD_SIZE) {
        SPDLOG_WARN("Payload overflow, waiting for worker thread");
        scheduler.waitWorker();
        payload_cursor = payload_start;
    }
    upload_start = payload_cursor;
}
}  // namespace render::vulkan