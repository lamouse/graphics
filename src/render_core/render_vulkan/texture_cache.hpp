#pragma once
#include "render_core/vulkan_common/memory_allocator.hpp"
#include "render_core/surface.hpp"
#include "render_core/texture/image_info.hpp"
#include "common/common_funcs.hpp"
#include "render_core/texture/types.hpp"
#include "render_core/render_vulkan/descriptor_pool.hpp"
#include "render_core/render_vulkan/render_pass.hpp"
#include "render_core/render_vulkan/update_descriptor.hpp"
#include "render_core/texture/image_view_base.hpp"
#include "render_core/texture/render_targets.h"
#include "render_core/texture.hpp"
#include "render_core/texture_cache/texture_cache.h"
import render.vulkan.common;
namespace render::vulkan {
class BlitImageHelper;

class StagingBufferPool;
struct StagingBufferRef;
class TextureImageView;
class TextureFramebuffer;
class TextureImage;
namespace scheduler {
class Scheduler;
}

class TextureCacheRuntime {
    public:
        explicit TextureCacheRuntime(const Device& device_, scheduler::Scheduler& scheduler_,
                                     MemoryAllocator& memory_allocator_,
                                     StagingBufferPool& staging_buffer_pool_,
                                     RenderPassCache& render_pass_cache_,
                                     resource::DescriptorPool& descriptor_pool,
                                     ComputePassDescriptorQueue& compute_pass_descriptor_queue);

        void Finish();

        auto UploadStagingBuffer(size_t size) -> StagingBufferRef;

        auto DownloadStagingBuffer(size_t size, bool deferred = false) -> StagingBufferRef;

        void FreeDeferredStagingBuffer(StagingBufferRef& ref);
        void TransitionImageLayout(TextureImage& image);

        void TickFrame();

        [[nodiscard]] auto GetDeviceLocalMemory() const -> u64;

        [[nodiscard]] auto GetDeviceMemoryUsage() const -> u64;

        [[nodiscard]] auto CanReportMemoryUsage() const -> bool;

        const Device& device;
        scheduler::Scheduler& scheduler;
        MemoryAllocator& memory_allocator;
        StagingBufferPool& staging_buffer_pool;
        RenderPassCache& render_pass_cache;
        std::array<std::vector<vk::Format>, surface::MaxPixelFormat> view_formats;
};

class TextureImage {
    public:
        explicit TextureImage(TextureCacheRuntime& runtime_,
                              const render::texture::ImageInfo& info_);
        explicit TextureImage();

        ~TextureImage();

        CLASS_NON_COPYABLE(TextureImage);
        TextureImage(TextureImage&&) = default;
        auto operator=(TextureImage&&) -> TextureImage& = default;

        void UploadMemory(vk::Buffer buffer, vk::DeviceSize offset,
                          std::span<const render::texture::BufferImageCopy> copies);

        void UploadMemory(const StagingBufferRef& map,
                          std::span<const render::texture::BufferImageCopy> copies);

        [[nodiscard]] auto Handle() const noexcept -> vk::Image { return *image; }

        [[nodiscard]] auto AspectMask() const noexcept -> vk::ImageAspectFlags {
            return aspect_mask;
        }

        [[nodiscard]] auto UsageFlags() const noexcept -> vk::ImageUsageFlags {
            return image.usageFlags();
        }
        auto getImageInfo() -> render::texture::ImageInfo { return info; }
        /// Returns true when the image is already initialized and mark it as initialized
        [[nodiscard]] auto ExchangeInitialization() noexcept -> bool {
            return std::exchange(initialized, true);
        }

    private:
        scheduler::Scheduler* scheduler{};
        TextureCacheRuntime* runtime{};
        render::texture::ImageInfo info;
        Image image;
        vk::ImageAspectFlags aspect_mask;
        bool initialized = false;
};

class TextureImageView : public render::texture::ImageViewBase {
    public:
        explicit TextureImageView(TextureCacheRuntime&, const texture::ImageViewInfo&,
                                  texture::ImageId, TextureImage&);
        explicit TextureImageView(TextureCacheRuntime&, const texture::NullImageViewParams&);

        ~TextureImageView();
        CLASS_NON_COPYABLE(TextureImageView);
        CLASS_DEFAULT_MOVEABLE(TextureImageView);

        [[nodiscard]] auto Handle(shader::TextureType texture_type) const noexcept
            -> vk::ImageView {
            return *image_views[static_cast<size_t>(texture_type)];
        }

        [[nodiscard]] auto ImageHandle() const noexcept -> vk::Image { return image_handle; }

        [[nodiscard]] auto RenderTarget() const noexcept -> vk::ImageView { return render_target; }

        [[nodiscard]] auto Samples() const noexcept -> vk::SampleCountFlagBits { return samples; }

    private:
        const Device* device = nullptr;

        std::array<ImageView, shader::NUM_TEXTURE_TYPES> image_views;
        vk::Image image_handle = VK_NULL_HANDLE;
        vk::ImageView render_target = VK_NULL_HANDLE;
        vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1;
};

class TextureSampler {
    public:
        explicit TextureSampler(TextureCacheRuntime&, SamplerReduction reduction,
                                int32_t imageMipLevels);
        explicit TextureSampler(TextureCacheRuntime& runtime, SamplerPreset preset);
        [[nodiscard]] auto Handle() const noexcept -> vk::Sampler { return *sampler; }

    private:
        Sampler sampler;
};

class TextureFramebuffer {
    public:
        explicit TextureFramebuffer(TextureCacheRuntime& runtime,
                                    std::span<TextureImageView*, texture::NUM_RT> color_buffers,
                                    TextureImageView* depth_buffer,
                                    const texture::RenderTargets& key);

        explicit TextureFramebuffer(TextureCacheRuntime& runtime, TextureImageView* color_buffer,
                                    TextureImageView* depth_buffer, vk::Extent2D extent);

        ~TextureFramebuffer();
        CLASS_NON_COPYABLE(TextureFramebuffer);
        CLASS_DEFAULT_MOVEABLE(TextureFramebuffer);

        void CreateFramebuffer(TextureCacheRuntime& runtime,
                               std::span<TextureImageView*, texture::NUM_RT> color_buffers,
                               TextureImageView* depth_buffer);

        [[nodiscard]] auto Handle() const noexcept -> vk::Framebuffer { return *framebuffer; }

        [[nodiscard]] auto RenderPass() const noexcept -> vk::RenderPass { return render_pass; }

        [[nodiscard]] auto RenderArea() const noexcept -> vk::Extent2D { return render_area; }

        [[nodiscard]] auto Samples() const noexcept -> vk::SampleCountFlagBits { return samples; }

        [[nodiscard]] auto NumColorBuffers() const noexcept -> u32 { return num_color_buffers; }

        [[nodiscard]] auto NumImages() const noexcept -> u32 { return num_images; }

        [[nodiscard]] auto Images() const noexcept -> const std::array<vk::Image, 9>& {
            return images;
        }

        [[nodiscard]] auto ImageViews() const noexcept
            -> const std::array<vk::ImageView, texture::NUM_RT>& {
            return color_views;
        }

        [[nodiscard]] auto DepthView() const noexcept -> vk::ImageView { return depth_view; }

        [[nodiscard]] auto ImageRanges() const noexcept
            -> const std::array<vk::ImageSubresourceRange, 9>& {
            return image_ranges;
        }

        [[nodiscard]] auto HasAspectColorBit(size_t index) const noexcept -> bool {
            return (image_ranges.at(rt_map[index]).aspectMask & vk::ImageAspectFlagBits::eColor) ==
                   vk::ImageAspectFlagBits::eColor;
        }

        [[nodiscard]] auto HasAspectDepthBit() const noexcept -> bool { return has_depth; }

        [[nodiscard]] auto HasAspectStencilBit() const noexcept -> bool { return has_stencil; }

        [[nodiscard]] auto IsRescaled() const noexcept -> bool { return is_rescaled; }

    private:
        VulkanFramebuffer framebuffer;
        vk::RenderPass render_pass;
        vk::Extent2D render_area;
        vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1;
        u32 num_color_buffers = 0;
        u32 num_images = 0;
        std::array<vk::Image, 9> images{};
        std::array<vk::ImageSubresourceRange, 9> image_ranges{};
        std::array<size_t, texture::NUM_RT> rt_map{};
        vk::ImageView depth_view;
        std::array<vk::ImageView, texture::NUM_RT> color_views{};
        bool has_depth{};
        bool has_stencil{};
        bool is_rescaled{};
};

struct TextureCacheParams {
        static constexpr bool ENABLE_VALIDATION = true;
        static constexpr bool FRAMEBUFFER_BLITS = false;
        static constexpr bool HAS_EMULATED_COPIES = false;
        static constexpr bool HAS_DEVICE_MEMORY_INFO = true;
        static constexpr bool IMPLEMENTS_ASYNC_DOWNLOADS = true;

        using Runtime = render::vulkan::TextureCacheRuntime;
        using Image = render::vulkan::TextureImage;
        using ImageView = render::vulkan::TextureImageView;
        using Sampler = render::vulkan::TextureSampler;
        using Framebuffer = render::vulkan::TextureFramebuffer;
        using AsyncBuffer = render::vulkan::StagingBufferRef;
        using BufferType = vk::Buffer;
};

using TextureCache = render::texture::TextureCache<TextureCacheParams>;

}  // namespace render::vulkan