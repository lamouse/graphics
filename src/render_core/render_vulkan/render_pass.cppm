module;
#include <vulkan/vulkan.hpp>
#include <unordered_map>
#include <mutex>
export module render.vulkan.render_pass;
import render.vulkan.common;
import render.surface.format;
export namespace render::vulkan {
struct RenderPassKey {
        auto operator==(const RenderPassKey& rhs) const noexcept -> bool = default;

        std::array<surface::PixelFormat, 8> color_formats;
        surface::PixelFormat depth_format;
        vk::SampleCountFlagBits samples;
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

            return h1 ^ (h2 << 1) ^ (h3 << 2);
        }
};
}  // namespace std
namespace render::vulkan {
export class RenderPassCache {
    public:
        explicit RenderPassCache(const Device& device_);

        auto get(const RenderPassKey& key) -> vk::RenderPass;

    private:
        const Device* device{};
        std::unordered_map<RenderPassKey, RenderPass> cache;
        std::mutex mutex;
};

}  // namespace render::vulkan
