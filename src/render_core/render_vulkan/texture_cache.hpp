#pragma once
#include "vulkan_common/vulkan_wrapper.hpp"
#include "vulkan_common/memory_allocator.hpp"
#include "common/slot_vector.hpp"
#include "surface.hpp"
#include "texture/image_base.hpp"
#include "texture/image_info.hpp"
#include "common/common_funcs.hpp"
#include "texture/types.hpp"
#include "descriptor_pool.hpp"
#include "render_pass.hpp"
#include "update_descriptor.hpp"
#include "compute_pass.hpp"
#include "texture/image_view_base.hpp"
#include "texture/render_targets.h"
#include "common/settings.hpp"

namespace render::vulkan {
class BlitImageHelper;
class Device;
class StagingBufferPool;
class StagingBufferRef;
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
                                     ComputePassDescriptorQueue& compute_pass_descriptor_queue,
                                     BlitImageHelper& blit_image_helper_);

        void Finish();

        StagingBufferRef UploadStagingBuffer(size_t size);

        StagingBufferRef DownloadStagingBuffer(size_t size, bool deferred = false);

        void FreeDeferredStagingBuffer(StagingBufferRef& ref);

        void TickFrame();

        u64 GetDeviceLocalMemory() const;

        u64 GetDeviceMemoryUsage() const;

        bool CanReportMemoryUsage() const;

        void BlitImage(TextureFramebuffer* dst_framebuffer, TextureImageView& dst,
                       TextureImageView& src, const render::texture::Region2D& dst_region,
                       const render::texture::Region2D& src_region);

        void CopyImage(TextureImage& dst, TextureImage& src,
                       std::span<const render::texture::ImageCopy> copies);

        void CopyImageMSAA(TextureImage& dst, TextureImage& src,
                           std::span<const render::texture::ImageCopy> copies);

        bool ShouldReinterpret(TextureImage& dst, TextureImage& src);

        void ReinterpretImage(TextureImage& dst, TextureImage& src,
                              std::span<const render::texture::ImageCopy> copies);

        void ConvertImage(TextureFramebuffer* dst, TextureImageView& dst_view,
                          TextureImageView& src_view);

        bool CanAccelerateImageUpload(TextureImage&) const noexcept { return false; }

        bool CanUploadMSAA() const noexcept {
            // TODO: Implement buffer to MSAA uploads
            return false;
        }

        void AccelerateImageUpload(TextureImage&, const StagingBufferRef&,
                                   std::span<const render::texture::SwizzleParameters>);

        void InsertUploadMemoryBarrier() {}

        void TransitionImageLayout(TextureImage& image);

        bool HasBrokenTextureViewFormats() const noexcept {
            // No known Vulkan driver has broken image views
            return false;
        }

        bool HasNativeBgr() const noexcept {
            // All known Vulkan drivers can natively handle BGR textures
            return true;
        }

        [[nodiscard]] vk::Buffer GetTemporaryBuffer(size_t needed_size);

        std::span<const vk::Format> ViewFormats(surface::PixelFormat format) {
            return view_formats[static_cast<std::size_t>(format)];
        }
        const settings::ResolutionScalingInfo resolution;
        void BarrierFeedbackLoop();
        BlitImageHelper& blit_image_helper;
        const Device& device;
        scheduler::Scheduler& scheduler;
        MemoryAllocator& memory_allocator;
        StagingBufferPool& staging_buffer_pool;
        RenderPassCache& render_pass_cache;
        std::optional<ASTCDecoderPass> astc_decoder_pass;
        std::unique_ptr<MSAACopyPass> msaa_copy_pass;
        std::array<std::vector<vk::Format>, surface::MaxPixelFormat> view_formats;
        static constexpr size_t indexing_slots = 8 * sizeof(size_t);
        std::array<Buffer, indexing_slots> buffers{};
};

class TextureImage : public render::texture::ImageBase {
    public:
        explicit TextureImage(TextureCacheRuntime& runtime_, const render::texture::ImageInfo& info,
                              GPUVAddr gpu_addr, VAddr cpu_addr);
        explicit TextureImage(const render::texture::NullImageParams&);

        ~TextureImage();

        CLASS_NON_COPYABLE(TextureImage);
        TextureImage(TextureImage&&) = default;
        auto operator=(TextureImage&&) -> TextureImage& = default;

        void UploadMemory(vk::Buffer buffer, vk::DeviceSize offset,
                          std::span<const render::texture::BufferImageCopy> copies);

        void UploadMemory(const StagingBufferRef& map,
                          std::span<const render::texture::BufferImageCopy> copies);

        void DownloadMemory(vk::Buffer buffer, size_t offset,
                            std::span<const render::texture::BufferImageCopy> copies);

        void DownloadMemory(std::span<vk::Buffer> buffers, std::span<size_t> offsets,
                            std::span<const render::texture::BufferImageCopy> copies);

        void DownloadMemory(const StagingBufferRef& map,
                            std::span<const render::texture::BufferImageCopy> copies);

        [[nodiscard]] auto Handle() const noexcept -> vk::Image { return *(this->*current_image); }

        [[nodiscard]] auto AspectMask() const noexcept -> vk::ImageAspectFlags {
            return aspect_mask;
        }

        [[nodiscard]] auto UsageFlags() const noexcept -> vk::ImageUsageFlags {
            return (this->*current_image).usageFlags();
        }

        /// Returns true when the image is already initialized and mark it as initialized
        [[nodiscard]] auto ExchangeInitialization() noexcept -> bool {
            return std::exchange(initialized, true);
        }

        auto StorageImageView(s32 level) noexcept -> vk::ImageView;

        auto IsRescaled() const noexcept -> bool;

        auto ScaleUp(bool ignore = false) -> bool;

        auto ScaleDown(bool ignore = false) -> bool;

    private:
        auto BlitScaleHelper(bool scale_up) -> bool;

        [[nodiscard]] auto NeedsScaleHelper() const -> bool;

        scheduler::Scheduler* scheduler{};
        TextureCacheRuntime* runtime{};
        Image original_image;
        Image scaled_image;

        // Use a pointer to field because it is relative, so that the object can be
        // moved without breaking the reference.
        Image TextureImage::* current_image{};

        std::vector<ImageView> storage_image_views;
        vk::ImageAspectFlags aspect_mask;
        bool initialized = false;

        std::unique_ptr<TextureFramebuffer> scale_framebuffer;
        std::unique_ptr<TextureImageView> scale_view;

        std::unique_ptr<TextureFramebuffer> normal_framebuffer;
        std::unique_ptr<TextureImageView> normal_view;
};

class TextureImageView : public render::texture::ImageViewBase {
    public:
        explicit TextureImageView(TextureCacheRuntime&, const texture::ImageViewInfo&,
                                  texture::ImageId, TextureImage&);
        explicit TextureImageView(TextureCacheRuntime&, const texture::ImageViewInfo&,
                                  texture::ImageId, TextureImage&,
                                  const common::SlotVector<TextureImage>&);
        explicit TextureImageView(TextureCacheRuntime&, const texture::ImageInfo&,
                                  const texture::ImageViewInfo&, GPUVAddr);
        explicit TextureImageView(TextureCacheRuntime&, const texture::NullImageViewParams&);

        ~TextureImageView();
        CLASS_NON_COPYABLE(TextureImageView);
        CLASS_DEFAULT_MOVEABLE(TextureImageView);

        [[nodiscard]] auto DepthView() -> vk::ImageView;

        [[nodiscard]] auto StencilView() -> vk::ImageView;

        [[nodiscard]] auto ColorView() -> vk::ImageView;

        [[nodiscard]] auto StorageView(shader::TextureType texture_type,
                                       shader::ImageFormat image_format) -> vk::ImageView;

        [[nodiscard]] bool IsRescaled() const noexcept;

        [[nodiscard]] vk::ImageView Handle(shader::TextureType texture_type) const noexcept {
            return *image_views[static_cast<size_t>(texture_type)];
        }

        [[nodiscard]] vk::Image ImageHandle() const noexcept { return image_handle; }

        [[nodiscard]] vk::ImageView RenderTarget() const noexcept { return render_target; }

        [[nodiscard]] auto Samples() const noexcept -> vk::SampleCountFlagBits { return samples; }

        [[nodiscard]] GPUVAddr GpuAddr() const noexcept { return gpu_addr; }

        [[nodiscard]] u32 BufferSize() const noexcept { return buffer_size; }

    private:
        struct StorageViews {
                std::array<ImageView, shader::NUM_TEXTURE_TYPES> signeds;
                std::array<ImageView, shader::NUM_TEXTURE_TYPES> unsigneds;
        };

        [[nodiscard]] ImageView MakeView(vk::Format vk_format, vk::ImageAspectFlags aspect_mask);

        const Device* device = nullptr;
        const common::SlotVector<TextureImage>* slot_images = nullptr;

        std::array<ImageView, shader::NUM_TEXTURE_TYPES> image_views;
        std::unique_ptr<StorageViews> storage_views;
        ImageView depth_view;
        ImageView stencil_view;
        ImageView color_view;
        Image null_image;
        vk::Image image_handle = VK_NULL_HANDLE;
        vk::ImageView render_target = VK_NULL_HANDLE;
        vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1;
        u32 buffer_size = 0;
};

class TextureImageAlloc : public texture::ImageAllocBase {};

class TextureSampler {
    public:
        explicit TextureSampler(TextureCacheRuntime&, SamplerReduction reduction,
                                float imageMipLevels);

        [[nodiscard]] vk::Sampler Handle() const noexcept { return *sampler; }

        [[nodiscard]] vk::Sampler HandleWithDefaultAnisotropy() const noexcept {
            return *sampler_default_anisotropy;
        }

        [[nodiscard]] auto HasAddedAnisotropy() const noexcept -> bool {
            return static_cast<bool>(sampler_default_anisotropy);
        }

    private:
        Sampler sampler;
        Sampler sampler_default_anisotropy;
};

class TextureFramebuffer {
    public:
        explicit TextureFramebuffer(TextureCacheRuntime& runtime,
                                    std::span<TextureImageView*, texture::NUM_RT> color_buffers,
                                    TextureImageView* depth_buffer,
                                    const texture::RenderTargets& key);

        explicit TextureFramebuffer(TextureCacheRuntime& runtime, TextureImageView* color_buffer,
                                    TextureImageView* depth_buffer, vk::Extent2D extent,
                                    bool is_rescaled);

        ~TextureFramebuffer();

        CLASS_NON_COPYABLE(TextureFramebuffer);

        void CreateFramebuffer(TextureCacheRuntime& runtime,
                               std::span<TextureImageView*, texture::NUM_RT> color_buffers,
                               TextureImageView* depth_buffer, bool is_rescaled = false);

        [[nodiscard]] vk::Framebuffer Handle() const noexcept { return *framebuffer; }

        [[nodiscard]] vk::RenderPass RenderPass() const noexcept { return renderpass; }

        [[nodiscard]] vk::Extent2D RenderArea() const noexcept { return render_area; }

        [[nodiscard]] vk::SampleCountFlagBits Samples() const noexcept { return samples; }

        [[nodiscard]] u32 NumColorBuffers() const noexcept { return num_color_buffers; }

        [[nodiscard]] u32 NumImages() const noexcept { return num_images; }

        [[nodiscard]] const std::array<vk::Image, 9>& Images() const noexcept { return images; }

        [[nodiscard]] const std::array<vk::ImageSubresourceRange, 9>& ImageRanges() const noexcept {
            return image_ranges;
        }

        [[nodiscard]] auto HasAspectColorBit(size_t index) const noexcept -> bool {
            return (image_ranges.at(rt_map[index]).aspectMask & vk::ImageAspectFlagBits::eColor) ==
                   vk::ImageAspectFlagBits::eColor;
        }

        [[nodiscard]] bool HasAspectDepthBit() const noexcept { return has_depth; }

        [[nodiscard]] bool HasAspectStencilBit() const noexcept { return has_stencil; }

        [[nodiscard]] bool IsRescaled() const noexcept { return is_rescaled; }

    private:
        Framebuffer framebuffer;
        vk::RenderPass renderpass{};
        vk::Extent2D render_area{};
        vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1;
        u32 num_color_buffers = 0;
        u32 num_images = 0;
        std::array<vk::Image, 9> images{};
        std::array<vk::ImageSubresourceRange, 9> image_ranges{};
        std::array<size_t, texture::NUM_RT> rt_map{};
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
        using ImageAlloc = render::vulkan::TextureImageAlloc;
        using ImageView = render::vulkan::TextureImageView;
        using Sampler = render::vulkan::TextureSampler;
        using Framebuffer = render::vulkan::TextureFramebuffer;
        using AsyncBuffer = render::vulkan::StagingBufferRef;
        using BufferType = vk::Buffer;
};

}  // namespace render::vulkan