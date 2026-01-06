#include "texture_cache.hpp"
#include <spdlog/spdlog.h>
#include "vulkan_common/device.hpp"
#include "scheduler.hpp"
#include "staging_buffer_pool.hpp"
#include "texture/formatter.h"
#include "common/assert.hpp"
#undef MemoryBarrier

namespace render::vulkan {
namespace {
constexpr auto ConvertBorderColor(const std::array<float, 4>& color) -> vk::BorderColor {
    if (color == std::array<float, 4>{0, 0, 0, 0}) {
        return vk::BorderColor::eFloatTransparentBlack;
    }
    if (color == std::array<float, 4>{0, 0, 0, 1}) {
        return vk::BorderColor::eFloatOpaqueBlack;
    }
    if (color == std::array<float, 4>{1, 1, 1, 1}) {
        return vk::BorderColor::eFloatOpaqueWhite;
    }
    if (color[0] + color[1] + color[2] > 1.35F) {
        // If color elements are brighter than roughly 0.5 average, use white border
        return vk::BorderColor::eFloatOpaqueWhite;
    }
    if (color[3] > 0.5F) {
        return vk::BorderColor::eFloatOpaqueBlack;
    }
    return vk::BorderColor::eFloatTransparentBlack;
}

auto samplerReduction(SamplerReduction reduction) -> vk::SamplerReductionMode {
    switch (reduction) {
        case SamplerReduction::WeightedAverage:
            return vk::SamplerReductionMode::eWeightedAverage;
        case SamplerReduction::Min:
            return vk::SamplerReductionMode::eMin;
        case SamplerReduction::Max:
            return vk::SamplerReductionMode::eMax;
    }
    SPDLOG_ERROR("Invalid sampler mode={}", static_cast<int>(reduction));
    return vk::SamplerReductionMode::eWeightedAverage;
}

[[nodiscard]] auto ImageAspectMask(surface::PixelFormat format) -> vk::ImageAspectFlags {
    switch (surface::GetFormatType(format)) {
        case surface::SurfaceType::ColorTexture:
            return vk::ImageAspectFlagBits::eColor;
        case surface::SurfaceType::Depth:
            return vk::ImageAspectFlagBits::eDepth;
        case surface::SurfaceType::Stencil:
            return vk::ImageAspectFlagBits::eStencil;
        case surface::SurfaceType::DepthStencil:
            return vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
        default:
            ASSERT_MSG(false, "Invalid surface type");
            return vk::ImageAspectFlags{};
    }
}

[[nodiscard]] auto MakeSubresourceRange(vk::ImageAspectFlags aspect_mask,
                                        const texture::SubresourceRange& range)
    -> vk::ImageSubresourceRange {
    return vk::ImageSubresourceRange()
        .setAspectMask(aspect_mask)
        .setBaseMipLevel(range.base.level)
        .setLevelCount(range.extent.levels)
        .setBaseArrayLayer(range.base.layer)
        .setLayerCount(range.extent.layers);
}

[[nodiscard]] auto TransformBufferImageCopies(std::span<const texture::BufferImageCopy> copies,
                                              size_t buffer_offset,
                                              vk::ImageAspectFlags aspect_mask)
    -> boost::container::small_vector<vk::BufferImageCopy, 16> {
    struct Maker {
            auto operator()(const texture::BufferImageCopy& copy) const -> vk::BufferImageCopy {
                return vk::BufferImageCopy()
                    .setBufferOffset(copy.buffer_offset + buffer_offset)
                    .setBufferRowLength(copy.buffer_row_length)
                    .setBufferImageHeight(copy.buffer_image_height)
                    .setImageSubresource(
                        vk::ImageSubresourceLayers()
                            .setAspectMask(aspect_mask)
                            .setMipLevel(static_cast<u32>(copy.image_subresource.base_level))
                            .setBaseArrayLayer(static_cast<u32>(copy.image_subresource.base_layer))
                            .setLayerCount(static_cast<u32>(copy.image_subresource.num_layers)))
                    .setImageOffset(vk::Offset3D()
                                        .setX(copy.image_offset.x)
                                        .setY(copy.image_offset.y)
                                        .setZ(copy.image_offset.z))
                    .setImageExtent(vk::Extent3D()
                                        .setWidth(copy.image_extent.width)
                                        .setHeight(copy.image_extent.height)
                                        .setDepth(copy.image_extent.depth));
            }
            size_t buffer_offset;
            vk::ImageAspectFlags aspect_mask;
    };

    if (aspect_mask == (vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil)) {
        boost::container::small_vector<vk::BufferImageCopy, 16> result(copies.size() * 2);
        std::ranges::transform(
            copies, result.begin(),
            Maker{.buffer_offset = buffer_offset, .aspect_mask = vk::ImageAspectFlagBits::eDepth});
        std::ranges::transform(copies, result.begin() + copies.size(),
                               Maker{.buffer_offset = buffer_offset,
                                     .aspect_mask = vk::ImageAspectFlagBits::eStencil});
        return result;
    }
    boost::container::small_vector<vk::BufferImageCopy, 16> result(copies.size());
    std::ranges::transform(copies, result.begin(),
                           Maker{.buffer_offset = buffer_offset, .aspect_mask = aspect_mask});
    return result;
}

struct RangedBarrierRange {
        u32 min_mip = std::numeric_limits<u32>::max();
        u32 max_mip = std::numeric_limits<u32>::min();
        u32 min_layer = std::numeric_limits<u32>::max();
        u32 max_layer = std::numeric_limits<u32>::min();

        void AddLayers(const VkImageSubresourceLayers& layers) {
            min_mip = std::min(min_mip, layers.mipLevel);
            max_mip = std::max(max_mip, layers.mipLevel + 1);
            min_layer = std::min(min_layer, layers.baseArrayLayer);
            max_layer = std::max(max_layer, layers.baseArrayLayer + layers.layerCount);
        }

        [[nodiscard]] auto SubresourceRange(vk::ImageAspectFlags aspect_mask) const noexcept
            -> vk::ImageSubresourceRange {
            return vk::ImageSubresourceRange()
                .setAspectMask(aspect_mask)
                .setBaseMipLevel(min_mip)
                .setLevelCount(max_mip - min_mip)
                .setBaseArrayLayer(min_layer)
                .setLayerCount(max_layer - min_layer);
        }
};

[[nodiscard]] auto MakeSubresourceRange(const TextureImageView* image_view)
    -> vk::ImageSubresourceRange {
    texture::SubresourceRange range = image_view->range;

    return MakeSubresourceRange(ImageAspectMask(image_view->format), range);
}

[[nodiscard]] auto ImageViewType(shader::TextureType type) -> vk::ImageViewType {
    switch (type) {
        case shader::TextureType::Color1D:
            return vk::ImageViewType::e1D;
        case shader::TextureType::Color2D:
        case shader::TextureType::Color2DRect:
            return vk::ImageViewType::e2D;
        case shader::TextureType::ColorCube:
            return vk::ImageViewType::eCube;
        case shader::TextureType::Color3D:
            return vk::ImageViewType::e3D;
        case shader::TextureType::ColorArray1D:
            return vk::ImageViewType::e1DArray;
        case shader::TextureType::ColorArray2D:
            return vk::ImageViewType::e2DArray;
        case shader::TextureType::ColorArrayCube:
            return vk::ImageViewType::eCubeArray;
        case shader::TextureType::Buffer:
            SPDLOG_ERROR("Texture buffers can't be image views");
            return vk::ImageViewType::e1D;
    }
    SPDLOG_ERROR("Invalid image view type=");
    return vk::ImageViewType::e2D;
}

[[nodiscard]] auto ImageUsageFlags(const FormatInfo& info, surface::PixelFormat format)
    -> vk::ImageUsageFlags {
    vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eTransferSrc |
                                vk::ImageUsageFlagBits::eTransferDst |
                                vk::ImageUsageFlagBits::eSampled;
    if (info.attachable) {
        switch (surface::GetFormatType(format)) {
            case surface::SurfaceType::ColorTexture:
                usage |= vk::ImageUsageFlagBits::eColorAttachment;
                break;
            case surface::SurfaceType::Depth:
            case surface::SurfaceType::Stencil:
            case surface::SurfaceType::DepthStencil:
                usage |= vk::ImageUsageFlagBits::eDepthStencilAttachment;
                break;
            default:
                assert(false && "Invalid surface type");
                break;
        }
    }
    if (info.storage) {
        usage |= vk::ImageUsageFlagBits::eStorage;
    }
    return usage;
}

[[nodiscard]] auto ConvertImageType(const texture::ImageType type) -> vk::ImageType {
    switch (type) {
        case texture::ImageType::e2D:
            return vk::ImageType::e2D;
        case texture::ImageType::e3D:
            return vk::ImageType::e3D;
    }
    assert(false && "ConvertImageType Invalid image type=");
    return {};
}

[[nodiscard]] auto ConvertSampleCount(u32 num_samples) -> vk::SampleCountFlagBits {
    switch (num_samples) {
        case 1:
            return vk::SampleCountFlagBits::e1;
        case 2:
            return vk::SampleCountFlagBits::e2;
        case 4:
            return vk::SampleCountFlagBits::e4;
        case 8:
            return vk::SampleCountFlagBits::e8;
        case 16:
            return vk::SampleCountFlagBits::e16;
        default:
            SPDLOG_ERROR("Invalid number of samples={} will return vk::SampleCountFlagBits::e1",
                         num_samples);
            return vk::SampleCountFlagBits::e1;
    }
}

void CopyBufferToImage(vk::CommandBuffer cmdbuf, vk::Buffer src_buffer, vk::Image image,
                       vk::ImageAspectFlags aspect_mask, bool is_initialized,
                       std::span<const vk::BufferImageCopy> copies) {
    static constexpr vk::AccessFlags WRITE_ACCESS_FLAGS =
        vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eColorAttachmentWrite |
        vk::AccessFlagBits::eDepthStencilAttachmentWrite;
    static constexpr vk::AccessFlags READ_ACCESS_FLAGS =
        vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eColorAttachmentRead |
        vk::AccessFlagBits::eDepthStencilAttachmentRead;

    const vk::ImageMemoryBarrier read_barrier =
        vk::ImageMemoryBarrier()
            .setSrcAccessMask(WRITE_ACCESS_FLAGS)
            .setDstAccessMask(vk::AccessFlagBits::eTransferWrite)
            .setOldLayout(is_initialized ? vk::ImageLayout::eGeneral : vk::ImageLayout::eUndefined)
            .setNewLayout(vk::ImageLayout::eTransferDstOptimal)
            .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setImage(image)
            .setSubresourceRange(vk::ImageSubresourceRange()
                                     .setAspectMask(aspect_mask)
                                     .setBaseMipLevel(0)
                                     .setLevelCount(VK_REMAINING_MIP_LEVELS)
                                     .setBaseArrayLayer(0)
                                     .setLayerCount(VK_REMAINING_ARRAY_LAYERS));
    const vk::ImageMemoryBarrier write_barrier =
        vk::ImageMemoryBarrier()
            .setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
            .setDstAccessMask(WRITE_ACCESS_FLAGS | READ_ACCESS_FLAGS)
            .setOldLayout(vk::ImageLayout::eTransferDstOptimal)
            .setNewLayout(vk::ImageLayout::eGeneral)
            .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setImage(image)
            .setSubresourceRange(vk::ImageSubresourceRange()
                                     .setAspectMask(aspect_mask)
                                     .setBaseMipLevel(0)
                                     .setLevelCount(VK_REMAINING_MIP_LEVELS)
                                     .setBaseArrayLayer(0)
                                     .setLayerCount(VK_REMAINING_ARRAY_LAYERS));
    cmdbuf.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands,
                           vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, read_barrier);
    cmdbuf.copyBufferToImage(src_buffer, image, vk::ImageLayout::eTransferDstOptimal, copies);
    // TODO: Move this to another API
    cmdbuf.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                           vk::PipelineStageFlagBits::eAllCommands, {}, {}, {}, write_barrier);
}

[[nodiscard]] auto MakeImageCreateInfo(const Device& device, const texture::ImageInfo& info)
    -> vk::ImageCreateInfo {
    const bool is_2d = (info.type == texture::ImageType::e2D);
    const bool is_3d = (info.type == texture::ImageType::e3D);
    const auto format_info = device.surfaceFormat(FormatType::Optimal, false, info.format);
    vk::ImageCreateFlags flags{};
    if (is_2d && info.resources.layers >= 6 && info.size.width == info.size.height &&
        !device.hasBrokenCubeImageCompatibility()) {
        flags |= vk::ImageCreateFlagBits::eCubeCompatible;
    }

    // fix moltenVK issues with some 3D games
    auto usage = ImageUsageFlags(format_info, info.format);
    if (is_3d) {
        flags |= vk::ImageCreateFlagBits::e2DArrayCompatible;
        // Force usage to be VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT only on MoltenVK
        if (device.isMoltenVK()) {
            usage = vk::ImageUsageFlagBits::eColorAttachment;
        }
    }
    return vk::ImageCreateInfo()
        .setFlags(flags)
        .setImageType(ConvertImageType(info.type))
        .setFormat(format_info.format)
        .setExtent(vk::Extent3D()
                       .setWidth(info.size.width)
                       .setHeight(info.size.height)
                       .setDepth(info.size.depth))
        .setMipLevels(info.resources.levels)
        .setArrayLayers(static_cast<u32>(info.resources.layers))
        .setSamples(ConvertSampleCount(info.num_samples))
        .setTiling(vk::ImageTiling::eOptimal)
        .setUsage(usage)
        .setSharingMode(vk::SharingMode::eExclusive)
        .setPQueueFamilyIndices(nullptr)
        .setInitialLayout(vk::ImageLayout::eUndefined);
}

[[nodiscard]] auto MakeImage(const Device& device, const MemoryAllocator& allocator,
                             const texture::ImageInfo& info,
                             std::span<const vk::Format> view_formats = {}) -> Image {
    vk::ImageCreateInfo image_ci = MakeImageCreateInfo(device, info);
    const vk::ImageFormatListCreateInfo image_format_list =
        vk::ImageFormatListCreateInfo().setViewFormats(view_formats);

    if (view_formats.size() > 1) {
        image_ci.flags |= vk::ImageCreateFlagBits::eMutableFormat;
        if (device.isKhrImageFormatListSupported()) {
            image_ci.pNext = &image_format_list;
        }
    }
    return allocator.createImage(image_ci);
}

}  // namespace

TextureCacheRuntime::TextureCacheRuntime(const Device& device_, scheduler::Scheduler& scheduler_,
                                         MemoryAllocator& memory_allocator_,
                                         StagingBufferPool& staging_buffer_pool_,
                                         RenderPassCache& render_pass_cache_,
                                         resource::DescriptorPool& descriptor_pool,
                                         ComputePassDescriptorQueue& compute_pass_descriptor_queue)
    : device(device_),
      scheduler(scheduler_),
      memory_allocator(memory_allocator_),
      staging_buffer_pool(staging_buffer_pool_),
      render_pass_cache(render_pass_cache_) {}

void TextureCacheRuntime::Finish() { scheduler.finish(); }

auto TextureCacheRuntime::UploadStagingBuffer(size_t size) -> StagingBufferRef {
    return staging_buffer_pool.Request(size, MemoryUsage::Upload);
}

auto TextureCacheRuntime::DownloadStagingBuffer(size_t size, bool deferred) -> StagingBufferRef {
    return staging_buffer_pool.Request(size, MemoryUsage::Download, deferred);
}

void TextureCacheRuntime::FreeDeferredStagingBuffer(StagingBufferRef& ref) {
    staging_buffer_pool.FreeDeferred(ref);
}

auto TextureCacheRuntime::GetDeviceLocalMemory() const -> u64 {
    return device.getDeviceLocalMemory();
}

auto TextureCacheRuntime::GetDeviceMemoryUsage() const -> u64 {
    return device.getDeviceMemoryUsage();
}

auto TextureCacheRuntime::CanReportMemoryUsage() const -> bool {
    return device.canReportMemoryUsage();
}

void TextureCacheRuntime::TickFrame() {}

void TextureCacheRuntime::TransitionImageLayout(TextureImage& image) {
    if (!image.ExchangeInitialization()) {
        vk::ImageMemoryBarrier barrier =
            vk::ImageMemoryBarrier()
                .setSrcAccessMask(vk::AccessFlagBits::eNone)
                .setDstAccessMask(vk::AccessFlagBits::eMemoryRead |
                                  vk::AccessFlagBits::eMemoryWrite)
                .setOldLayout(vk::ImageLayout::eUndefined)
                .setNewLayout(vk::ImageLayout::eGeneral)
                .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
                .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
                .setImage(image.Handle())
                .setSubresourceRange(vk::ImageSubresourceRange()
                                         .setAspectMask(image.AspectMask())
                                         .setBaseMipLevel(0)
                                         .setLevelCount(VK_REMAINING_MIP_LEVELS)
                                         .setBaseArrayLayer(0)
                                         .setLayerCount(VK_REMAINING_ARRAY_LAYERS));
        scheduler.requestOutsideRenderOperationContext();
        scheduler.record([barrier](vk::CommandBuffer cmdbuf) {
            cmdbuf.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe,
                                   vk::PipelineStageFlagBits::eAllCommands, {}, {}, {}, barrier);
        });
    }
}

TextureImage::TextureImage(TextureCacheRuntime& runtime_, const texture::ImageInfo& info_)
    : scheduler{&runtime_.scheduler},
      runtime{&runtime_},
      info(info_),
      image(MakeImage(runtime_.device, runtime_.memory_allocator, info_)),
      aspect_mask(ImageAspectMask(info_.format)) {
    if (runtime->device.hasDebuggingToolAttached()) {
        image.SetObjectNameEXT(texture::Name(info).c_str());
    }
}

TextureImage::TextureImage() = default;
TextureImage::~TextureImage() = default;
void TextureImage::UploadMemory(vk::Buffer buffer, vk::DeviceSize offset,
                                std::span<const render::texture::BufferImageCopy> copies) {
    scheduler->requestOutsideRenderOperationContext();
    auto vk_copies = TransformBufferImageCopies(copies, offset, aspect_mask);
    const vk::Buffer src_buffer = buffer;
    const vk::Image vk_image = *image;
    const vk::ImageAspectFlags vk_aspect_mask = aspect_mask;
    const bool is_initialized = std::exchange(initialized, true);
    scheduler->record([src_buffer, vk_image, vk_aspect_mask, is_initialized,
                       vk_copies](vk::CommandBuffer cmdbuf) {
        CopyBufferToImage(cmdbuf, src_buffer, vk_image, vk_aspect_mask, is_initialized, vk_copies);
    });
}

void TextureImage::UploadMemory(const StagingBufferRef& map,
                                std::span<const texture::BufferImageCopy> copies) {
    UploadMemory(map.buffer, map.offset, copies);
}

TextureImageView::TextureImageView(TextureCacheRuntime& runtime, const texture::ImageViewInfo& info,
                                   texture::ImageId image_id_, TextureImage& image)
    : render::texture::ImageViewBase(info, image.getImageInfo(), image_id_),
      device{&runtime.device},
      image_handle{image.Handle()},
      samples(ConvertSampleCount(image.getImageInfo().num_samples)) {
    using shader::TextureType;
    const vk::ImageAspectFlags aspect_mask = ImageAspectMask(info.format);
    const auto format_info = device->surfaceFormat(FormatType::Optimal, true, format);
    const vk::ImageViewUsageCreateInfo image_view_usage{image.UsageFlags()};
    const vk::ImageViewCreateInfo create_info =
        vk::ImageViewCreateInfo()
            .setImage(image.Handle())
            .setFormat(format_info.format)
            .setComponents(vk::ComponentMapping()
                               .setR(vk::ComponentSwizzle::eIdentity)
                               .setG(vk::ComponentSwizzle::eIdentity)
                               .setB(vk::ComponentSwizzle::eIdentity)
                               .setA(vk::ComponentSwizzle::eIdentity))
            .setSubresourceRange(MakeSubresourceRange(aspect_mask, info.range))
            .setPNext(&image_view_usage);
    const auto create = [&](shader::TextureType tex_type, std::optional<u32> num_layers) -> void {
        vk::ImageViewCreateInfo ci{create_info};
        ci.viewType = ImageViewType(tex_type);
        if (num_layers) {
            ci.subresourceRange.layerCount = *num_layers;
        }
        ImageView handle = device->logical().CreateImageView(ci);
        if (device->hasDebuggingToolAttached()) {
            handle.SetObjectNameEXT(texture::Name(*this).c_str());
        }
        // NOLINTNEXTLINE
        image_views[static_cast<size_t>(tex_type)] = std::move(handle);
    };
    switch (info.type) {
        case texture::ImageViewType::e2D:
            create(shader::TextureType::Color2D, 1);
            render_target = Handle(shader::TextureType::Color2D);
            break;
        case texture::ImageViewType::e2DArray:
            create(shader::TextureType::ColorArray2D, std::nullopt);
            render_target = Handle(shader::TextureType::ColorArray2D);
            break;
        case texture::ImageViewType::e3D:
            create(TextureType::Color3D, std::nullopt);
            render_target = Handle(shader::TextureType::Color3D);
            break;
        case texture::ImageViewType::Cube:
            create(TextureType::ColorCube, 6);
            render_target = Handle(shader::TextureType::ColorCube);
            break;
        case texture::ImageViewType::CubeArray:
            create(TextureType::ColorCube, 6);
            create(TextureType::ColorArrayCube, std::nullopt);
            render_target = Handle(shader::TextureType::ColorArrayCube);
            break;
    }
}

TextureImageView::TextureImageView(TextureCacheRuntime& runtime,
                                   const texture::NullImageViewParams& params)
    : texture::ImageViewBase{params}, device{&runtime.device} {}

TextureImageView::~TextureImageView() = default;

// TODO 这些需要修改
TextureSampler::TextureSampler(TextureCacheRuntime& runtime, SamplerReduction reduction,
                               int32_t imageMipLevels) {
    const auto& device = runtime.device;
    const bool arbitrary_borders = runtime.device.isExtCustomBorderColorSupported();
    auto border_color = std::array<float, 4>{0, 0, 0, 0};
    vk::ClearColorValue color{std::bit_cast<vk::ClearColorValue>(border_color)};
    const vk::SamplerCustomBorderColorCreateInfoEXT border_ci =
        vk::SamplerCustomBorderColorCreateInfoEXT().setCustomBorderColor(color).setFormat(
            vk::Format::eUndefined);

    const void* pnext = nullptr;
    if (arbitrary_borders) {
        pnext = &border_ci;
    }
    const vk::SamplerReductionModeCreateInfoEXT reduction_ci{samplerReduction(reduction), pnext};
    if (runtime.device.isExtSamplerFilterMinmaxSupported()) {
        pnext = &reduction_ci;
    } else if (reduction_ci.reductionMode != vk::SamplerReductionMode::eWeightedAverage) {
        SPDLOG_WARN("VK_EXT_sampler_filter_minmax is required");
    }
    // Some games have samplers with garbage. Sanitize them here.
    const f32 max_anisotropy = std::clamp(device.getMaxAnisotropy(), 1.0F, 16.0F);

    const auto create_sampler = [&](const f32 anisotropy) {
        ;
        return device.logical().CreateSampler(
            vk::SamplerCreateInfo()
                .setMagFilter(vk::Filter::eLinear)
                .setMinFilter(vk::Filter::eLinear)
                .setMipmapMode(vk::SamplerMipmapMode::eLinear)
                .setAddressModeU(vk::SamplerAddressMode::eRepeat)
                .setAddressModeV(vk::SamplerAddressMode::eRepeat)
                .setAddressModeW(vk::SamplerAddressMode::eRepeat)
                .setMipLodBias(.0f)
                .setAnisotropyEnable(static_cast<VkBool32>(anisotropy > 1.0F ? VK_TRUE : VK_FALSE))
                .setMaxAnisotropy(anisotropy)
                .setCompareEnable(VK_FALSE)
                .setCompareOp(::vk::CompareOp::eLessOrEqual)
                .setMinLod(.0f)
                .setMaxLod(static_cast<float>(imageMipLevels))
                .setBorderColor(arbitrary_borders ? vk::BorderColor::eFloatCustomEXT
                                                  : ConvertBorderColor(border_color))
                .setUnnormalizedCoordinates(VK_FALSE)
                .setPNext(pnext));
    };

    sampler = create_sampler(max_anisotropy);
}

TextureSampler::TextureSampler(TextureCacheRuntime& runtime, SamplerPreset preset) {
    const auto& device = runtime.device;
    const bool arbitrary_borders = runtime.device.isExtCustomBorderColorSupported();
    auto border_color = std::array<float, 4>{0, 0, 0, 0};
    vk::ClearColorValue color{std::bit_cast<vk::ClearColorValue>(border_color)};
    const vk::SamplerCustomBorderColorCreateInfoEXT border_ci =
        vk::SamplerCustomBorderColorCreateInfoEXT().setCustomBorderColor(color).setFormat(
            vk::Format::eUndefined);

    const void* pnext = nullptr;
    if (arbitrary_borders) {
        pnext = &border_ci;
    }
    vk::SamplerReductionModeCreateInfoEXT reduction_ci;
    auto setReduction = [&](SamplerReduction reduction) -> void {
        reduction_ci = vk::SamplerReductionModeCreateInfoEXT{samplerReduction(reduction), pnext};
        if (runtime.device.isExtSamplerFilterMinmaxSupported()) {
            pnext = &reduction_ci;
        } else if (reduction_ci.reductionMode != vk::SamplerReductionMode::eWeightedAverage) {
            SPDLOG_WARN("VK_EXT_sampler_filter_minmax is required");
        }
    };

    const auto create_sampler = [&](const f32 anisotropy, vk::Filter filter,
                                    vk::SamplerMipmapMode mipmapMode,
                                    vk::SamplerAddressMode samplerAddressMode) {
        ;
        return device.logical().CreateSampler(
            vk::SamplerCreateInfo()
                .setMagFilter(filter)
                .setMinFilter(filter)
                .setMipmapMode(mipmapMode)
                .setAddressModeU(samplerAddressMode)
                .setAddressModeV(samplerAddressMode)
                .setAddressModeW(samplerAddressMode)
                .setMipLodBias(.0f)
                .setAnisotropyEnable(static_cast<VkBool32>(anisotropy > 1.0F ? VK_TRUE : VK_FALSE))
                .setMaxAnisotropy(anisotropy)
                .setCompareEnable(VK_FALSE)
                .setCompareOp(::vk::CompareOp::eLessOrEqual)
                .setMinLod(.0F)
                .setMaxLod(VK_LOD_CLAMP_NONE)
                .setBorderColor(arbitrary_borders ? vk::BorderColor::eFloatCustomEXT
                                                  : ConvertBorderColor(border_color))
                .setUnnormalizedCoordinates(VK_FALSE)
                .setPNext(pnext));
    };

    // Some games have samplers with garbage. Sanitize them here.
    const f32 max_anisotropy = std::clamp(device.getMaxAnisotropy(), 1.0F, 16.0F);
    switch (preset) {
        case SamplerPreset::Linear:
            setReduction(SamplerReduction::WeightedAverage);
            sampler =
                create_sampler(max_anisotropy, vk::Filter::eLinear, vk::SamplerMipmapMode::eLinear,
                               vk::SamplerAddressMode::eRepeat);
            break;
        case SamplerPreset::Nearest:
            setReduction(SamplerReduction::WeightedAverage);
            sampler = create_sampler(max_anisotropy, vk::Filter::eNearest,
                                     vk::SamplerMipmapMode::eNearest,
                                     vk::SamplerAddressMode::eClampToEdge);
            break;
        case SamplerPreset::ShadowMin:
            setReduction(SamplerReduction::Min);
            sampler =
                create_sampler(max_anisotropy, vk::Filter::eLinear, vk::SamplerMipmapMode::eLinear,
                               vk::SamplerAddressMode::eClampToBorder);
            break;
        case SamplerPreset::HDRMax:
            setReduction(SamplerReduction::Max);
            sampler =
                create_sampler(max_anisotropy, vk::Filter::eLinear, vk::SamplerMipmapMode::eLinear,
                               vk::SamplerAddressMode::eClampToEdge);
            break;
        default:
            spdlog::warn("not set default SamplerPreset use Linear preset");
            setReduction(SamplerReduction::WeightedAverage);
            sampler =
                create_sampler(max_anisotropy, vk::Filter::eLinear, vk::SamplerMipmapMode::eLinear,
                               vk::SamplerAddressMode::eRepeat);
            break;
    }
}

TextureFramebuffer::TextureFramebuffer(TextureCacheRuntime& runtime,
                                       std::span<TextureImageView*, texture::NUM_RT> color_buffers,
                                       TextureImageView* depth_buffer,
                                       const texture::RenderTargets& key)
    : render_area{vk::Extent2D{key.size.width, key.size.height}} {
    CreateFramebuffer(runtime, color_buffers, depth_buffer);
    if (runtime.device.hasDebuggingToolAttached()) {
        framebuffer.SetObjectNameEXT(texture::Name(key).c_str());
    }
}

TextureFramebuffer::TextureFramebuffer(TextureCacheRuntime& runtime, TextureImageView* color_buffer,
                                       TextureImageView* depth_buffer, vk::Extent2D extent)
    : render_area{extent} {
    std::array<TextureImageView*, texture::NUM_RT> color_buffers{color_buffer};
    CreateFramebuffer(runtime, color_buffers, depth_buffer);
}

TextureFramebuffer::~TextureFramebuffer() = default;

void TextureFramebuffer::CreateFramebuffer(
    TextureCacheRuntime& runtime, std::span<TextureImageView*, texture::NUM_RT> color_buffers,
    TextureImageView* depth_buffer) {
    boost::container::small_vector<vk::ImageView, texture::NUM_RT + 1> attachments;
    RenderPassKey render_pass_key{};
    s32 num_layers = 1;
    u32 width = std::numeric_limits<u32>::max();
    u32 height = std::numeric_limits<u32>::max();
    for (size_t index = 0; index < texture::NUM_RT; ++index) {
        const TextureImageView* const color_buffer = color_buffers[index];
        if (!color_buffer) {
            render_pass_key.color_formats.at(index) = surface::PixelFormat::Invalid;
            continue;
        }
        color_views.at(index) = color_buffer->RenderTarget();
        width = color_buffer->size.width;
        height = color_buffer->size.height;
        attachments.push_back(color_buffer->RenderTarget());
        render_pass_key.color_formats.at(index) = color_buffer->format;
        num_layers = std::max(num_layers, color_buffer->range.extent.layers);
        images.at(num_images) = color_buffer->ImageHandle();
        image_ranges.at(num_images) = MakeSubresourceRange(color_buffer);
        rt_map.at(index) = num_images;
        samples = color_buffer->Samples();
        ++num_images;
    }
    const size_t num_colors = attachments.size();
    if (depth_buffer) {
        width = depth_buffer->size.width;
        height = depth_buffer->size.height;
        attachments.push_back(depth_buffer->RenderTarget());
        render_pass_key.depth_format = depth_buffer->format;
        num_layers = std::max(num_layers, depth_buffer->range.extent.layers);
        images.at(num_images) = depth_buffer->ImageHandle();
        const VkImageSubresourceRange subresource_range = MakeSubresourceRange(depth_buffer);
        image_ranges.at(num_images) = subresource_range;
        samples = depth_buffer->Samples();
        ++num_images;
        // NOLINTNEXTLINE
        has_depth = (subresource_range.aspectMask & VK_IMAGE_ASPECT_DEPTH_BIT) != 0;
        // NOLINTNEXTLINE
        has_stencil = (subresource_range.aspectMask & VK_IMAGE_ASPECT_STENCIL_BIT) != 0;
        depth_view = depth_buffer->RenderTarget();
    } else {
        render_pass_key.depth_format = surface::PixelFormat::Invalid;
    }
    render_pass_key.samples = samples;
    render_pass = runtime.render_pass_cache.get(render_pass_key);
    render_area.width = std::min(render_area.width, width);
    render_area.height = std::min(render_area.height, height);

    num_color_buffers = static_cast<u32>(num_colors);
    framebuffer = runtime.device.logical().createFramerBuffer(vk::FramebufferCreateInfo()
                                                                  .setRenderPass(render_pass)
                                                                  .setAttachments(attachments)
                                                                  .setWidth(render_area.width)
                                                                  .setHeight(render_area.height)
                                                                  .setLayers(num_layers));
}

[[nodiscard]] auto TextureFramebuffer::getRenderingRequest() const -> scheduler::RequestsRending {
    return scheduler::RequestsRending{.color_views = ImageViews(),
                                      .depth_view = DepthView(),
                                      .render_area = RenderArea(),
                                      .num_render_pass_images = NumImages(),
                                      .render_pass_images = Images(),
                                      .render_pass_image_ranges = ImageRanges()};
}
[[nodiscard]] auto TextureFramebuffer::getRenderPassRequest() const
    -> scheduler::RequestRenderPass {
    return scheduler::RequestRenderPass{.render_pass = RenderPass(),
                                        .framebuffer = Handle(),
                                        .render_area = RenderArea(),
                                        .num_render_pass_images = NumImages(),
                                        .render_pass_images = Images(),
                                        .render_pass_image_ranges = ImageRanges()};
}

}  // namespace render::vulkan