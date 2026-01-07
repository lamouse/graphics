module;
#include <spdlog/spdlog.h>
#include <vulkan/vulkan.hpp>
#include "vma.hpp"
module render.vulkan.common.MemoryAllocator;
import render.vulkan.common.wrapper;
import common;

namespace render::vulkan {
namespace {

[[nodiscard]] auto memoryUsagePreferredVmaFlags(MemoryUsage usage) -> vk::MemoryPropertyFlags {
    return usage != MemoryUsage::DeviceLocal ? vk::MemoryPropertyFlagBits::eHostVisible
                                             : vk::MemoryPropertyFlags{};
}
[[nodiscard]] auto memoryUsageVmaFlags(MemoryUsage usage) -> VmaAllocationCreateFlags {
    switch (usage) {
        case MemoryUsage::Upload:
        case MemoryUsage::Stream:
            return VMA_ALLOCATION_CREATE_MAPPED_BIT |
                   VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        case MemoryUsage::Download:
            return VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
        case MemoryUsage::DeviceLocal:
            return {};
    }
    return {};
}

[[nodiscard]] auto memoryUsageVma(MemoryUsage usage) -> VmaMemoryUsage {
    switch (usage) {
        case MemoryUsage::DeviceLocal:
        case MemoryUsage::Stream:
            return VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        case MemoryUsage::Upload:
        case MemoryUsage::Download:
            return VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
    }
    return VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
}
}  // namespace

MemoryAllocator::MemoryAllocator(const Device& device_)
    : device{device_},
      allocator{device_.getAllocator()},
      properties{device_.getPhysical().getMemoryProperties2().memoryProperties},
      buffer_image_granularity{
          device_.getPhysical().getProperties().limits.bufferImageGranularity} {
    // GPUs not supporting rebar may only have a region with less than 256MB host visible/device
    // local memory. In that case, opening 2 RenderDoc captures side-by-side is not possible due to
    // the heap running out of memory. With RenderDoc attached and only a small host/device region,
    // only allow the stream buffer in this memory heap.
    if (device_.hasDebuggingToolAttached()) {
        using namespace common::literals;
        ForEachDeviceLocalHostVisibleHeap(device, [this](size_t index, VkMemoryHeap& heap) {
            if (heap.size <= 256_MiB) {
                valid_memory_types &= ~(1u << index);
            }
        });
    }
}

MemoryAllocator::~MemoryAllocator() = default;

auto MemoryAllocator::createImage(const VkImageCreateInfo& ci) const -> Image {
    const VmaAllocationCreateInfo alloc_ci = {
        .flags = VMA_ALLOCATION_CREATE_WITHIN_BUDGET_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
        .requiredFlags = 0,
        .preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        .memoryTypeBits = 0,
        .pool = VK_NULL_HANDLE,
        .pUserData = nullptr,
        .priority = 0.f,
    };

    VkImage handle{};
    VmaAllocation allocation{};

    utils::check(static_cast<vk::Result>(
        vmaCreateImage(allocator, &ci, &alloc_ci, &handle, &allocation, nullptr)));

    return Image{handle, static_cast<vk::ImageUsageFlags>(ci.usage), device.getLogical(), allocator,
                 allocation};
}

auto MemoryAllocator::createBuffer(const VkBufferCreateInfo& ci, MemoryUsage usage) const
    -> Buffer {
    const VmaAllocationCreateInfo alloc_ci = {
        .flags = VMA_ALLOCATION_CREATE_WITHIN_BUDGET_BIT | memoryUsageVmaFlags(usage),
        .usage = memoryUsageVma(usage),
        .requiredFlags = 0,
        .preferredFlags = static_cast<VkMemoryPropertyFlags>(memoryUsagePreferredVmaFlags(usage)),
        .memoryTypeBits = usage == MemoryUsage::Stream ? 0u : valid_memory_types,
        .pool = VK_NULL_HANDLE,
        .pUserData = nullptr,
        .priority = 0.f,
    };

    VkBuffer handle{};
    VmaAllocationInfo alloc_info{};
    VmaAllocation allocation{};
    VkMemoryPropertyFlags property_flags{};

    utils::check(vmaCreateBuffer(allocator, &ci, &alloc_ci, &handle, &allocation, &alloc_info));
    vmaGetAllocationMemoryProperties(allocator, allocation, &property_flags);

    u8* data = reinterpret_cast<u8*>(alloc_info.pMappedData);
    const std::span<u8> mapped_data = data ? std::span<u8>{data, ci.size} : std::span<u8>{};
    const bool is_coherent = property_flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    return Buffer(handle, device.getLogical(), allocator, allocation, mapped_data, is_coherent);
}
}  // namespace render::vulkan