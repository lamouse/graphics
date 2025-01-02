#pragma once
#include <vulkan/vulkan.hpp>
#include <unordered_map>
#include <mutex>
namespace render::vulkan {
class Device;

struct RenderPassKey {
        auto operator==(const RenderPassKey& rhs) const noexcept -> bool = default;

        std::array<vk::Format, 8> color_formats;
        vk::Format depth_format;
        vk::SampleCountFlagBits samples;
        bool need_resolvet;
        bool save_depth;
        bool need_clear;
        bool need_store;
};
}  // namespace render::vulkan
namespace std {
template <>
struct hash<render::vulkan::RenderPassKey> {
        [[nodiscard]] auto operator()(const render::vulkan::RenderPassKey& key) const noexcept
            -> size_t {
            std::size_t h1 = 0;
            for (auto format : key.color_formats) {
                h1 ^= static_cast<uint32_t>(format);
            }
            std::size_t h2 = std::hash<int>()(static_cast<int>(key.depth_format));
            std::size_t h3 = std::hash<int>()(static_cast<int>(key.samples));
            std::size_t h4 = std::hash<bool>()(key.need_resolvet);
            std::size_t h5 = std::hash<bool>()(key.save_depth);
            std::size_t h6 = std::hash<bool>()(key.need_clear);
            std::size_t h7 = std::hash<bool>()(key.need_store);
            return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3) ^ (h5 << 4) ^ (h6 << 5) ^ (h7 << 6);
        }
};
}  // namespace std
namespace render::vulkan {
class RenderPassCache {
    public:
        explicit RenderPassCache(const Device& device_);

        auto get(const RenderPassKey& key) -> vk::RenderPass;

    private:
        const Device* device{};
        std::unordered_map<RenderPassKey, vk::RenderPass> cache;
        std::mutex mutex;
};

}  // namespace render::vulkan
