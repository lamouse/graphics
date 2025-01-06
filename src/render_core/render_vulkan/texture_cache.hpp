#pragma once
#include "vulkan_common/vulkan_wrapper.hpp"
#include "vulkan_common/memory_allocator.hpp"
#include "surface.hpp"
#include "texture/image_base.hpp"
#include "texture/image_info.hpp"
#include "common/common_funcs.hpp"
#include "texture/types.hpp"
#include "descriptor_pool.hpp"
#include "render_pass.hpp"
#include "update_descriptor.hpp"
#include "compute_pass.hpp"

namespace render::vulkan {
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
                                     ComputePassDescriptorQueue& compute_pass_descriptor_queue);

        void Finish();

        StagingBufferRef UploadStagingBuffer(size_t size);

        StagingBufferRef DownloadStagingBuffer(size_t size, bool deferred = false);

        void FreeDeferredStagingBuffer(StagingBufferRef& ref);

        void TickFrame();

        u64 GetDeviceLocalMemory() const;

        u64 GetDeviceMemoryUsage() const;

        bool CanReportMemoryUsage() const;

        void BlitImage(Framebuffer* dst_framebuffer, ImageView& dst, ImageView& src,
                       const render::texture::Region2D& dst_region,
                       const render::texture::Region2D& src_region);

        void CopyImage(TextureImage& dst, TextureImage& src,
                       std::span<const render::texture::ImageCopy> copies);

        void CopyImageMSAA(TextureImage& dst, TextureImage& src,
                           std::span<const render::texture::ImageCopy> copies);

        bool ShouldReinterpret(TextureImage& dst, TextureImage& src);

        void ReinterpretImage(TextureImage& dst, TextureImage& src,
                              std::span<const render::texture::ImageCopy> copies);

        void ConvertImage(Framebuffer* dst, TextureImageView& dst_view, TextureImageView& src_view);

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

        void BarrierFeedbackLoop();

        const Device& device;
        scheduler::Scheduler& scheduler;
        MemoryAllocator& memory_allocator;
        StagingBufferPool& staging_buffer_pool;
        RenderPassCache& render_pass_cache;
        std::unique_ptr<MSAACopyPass> msaa_copy_pass;
        std::array<std::vector<vk::Format>, surface::MaxPixelFormat> view_formats;

        static constexpr size_t indexing_slots = 8 * sizeof(size_t);
        std::array<Buffer, indexing_slots> buffers{};
};

class TextureImage : public render::texture::ImageBase {
    public:
        explicit TextureImage(const render::texture::ImageInfo& info, GPUVAddr gpu_addr,
                              VAddr cpu_addr);
        explicit TextureImage(const render::texture::NullImageParams&);

        ~TextureImage();

        CLASS_NON_COPYABLE(TextureImage);
        TextureImage(TextureImage&&) = default;
        auto operator=(TextureImage&&) -> TextureImage& = default;

        void UploadMemory(vk::Buffer buffer, vk::DeviceSize offset,
                          std::span<const render::texture::BufferImageCopy> copies);

        void UploadMemory(const StagingBufferRef& map,
                          std::span<const render::texture::BufferImageCopy> copies);

        void DownloadMemory(VkBuffer buffer, size_t offset,
                            std::span<const render::texture::BufferImageCopy> copies);

        void DownloadMemory(std::span<VkBuffer> buffers, std::span<size_t> offsets,
                            std::span<const render::texture::BufferImageCopy> copies);

        void DownloadMemory(const StagingBufferRef& map,
                            std::span<const render::texture::BufferImageCopy> copies);

        [[nodiscard]] vk::Image Handle() const noexcept { return *(*current_image); }

        [[nodiscard]] auto AspectMask() const noexcept -> vk::ImageAspectFlags {
            return aspect_mask;
        }

        [[nodiscard]] auto UsageFlags() const noexcept -> vk::ImageUsageFlags {
            return current_image->usageFlags();
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

        Image original_image;
        Image scaled_image;

        // Use a pointer to field because it is relative, so that the object can be
        // moved without breaking the reference.
        Image* current_image{};

        std::vector<ImageView> storage_image_views;
        vk::ImageAspectFlags aspect_mask;
        bool initialized = false;

        std::unique_ptr<TextureFramebuffer> scale_framebuffer;
        std::unique_ptr<TextureImageView> scale_view;

        std::unique_ptr<TextureFramebuffer> normal_framebuffer;
        std::unique_ptr<TextureImageView> normal_view;
};

}  // namespace render::vulkan