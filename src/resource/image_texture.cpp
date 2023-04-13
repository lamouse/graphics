#include "image_texture.hpp"
#include "../core/staging_buffer.hpp"
namespace resource::image {

ImageTexture::ImageTexture(core::Device& device, Image& image, ::vk::Format format):format_(format),device_(device.logicalDevice())
{
    imageMipLevels_ = image.getMipLevels();
    ImageInfo imgInfo = image.getImageInfo();
    imageMipLevels_ = image.getMipLevels();

    core::StagingBuffer stagingBuffer(device, image.size(), image.getData());

    device.createImage(imgInfo.width, imgInfo.height, imageMipLevels_, format_, ::vk::SampleCountFlagBits::e1, ::vk::ImageTiling::eOptimal, 
                    ::vk::ImageUsageFlagBits::eTransferSrc|::vk::ImageUsageFlagBits::eTransferDst |::vk::ImageUsageFlagBits::eSampled, 
                    ::vk::MemoryPropertyFlagBits::eDeviceLocal, image_, imageMemory_);

    transitionImageLayout(device, image_, format_, ::vk::ImageLayout::eUndefined, ::vk::ImageLayout::eTransferDstOptimal);

    device.excuteCmd([&](::vk::CommandBuffer cmdBuf){
        ::vk::BufferImageCopy region;
                ::vk::ImageSubresourceLayers imageSubsourceLayers;
        imageSubsourceLayers.setAspectMask(::vk::ImageAspectFlagBits::eColor)
                            .setMipLevel(0)
                            .setBaseArrayLayer(0)
                            .setLayerCount(1);
        region.setBufferOffset(0)
                .setBufferRowLength(0)
                .setBufferImageHeight(0)
                .setImageSubresource(imageSubsourceLayers)
                .setImageOffset({0, 0, 0})
                .setImageExtent({static_cast<uint32_t>(imgInfo.width), static_cast<uint32_t>(imgInfo.height), 1});
        cmdBuf.copyBufferToImage(stagingBuffer.getBuffer(), image_, ::vk::ImageLayout::eTransferDstOptimal, region);
    });

    transitionImageLayout(device, image_, format_, ::vk::ImageLayout::eUndefined, ::vk::ImageLayout::eTransferDstOptimal);
    generateMipmaps(device ,image_, imgInfo.width, imgInfo.height,  imageMipLevels_);
    createTextureImageView(device);
    createTextureSampler(device);
}

void ImageTexture::transitionImageLayout(core::Device& device, ::vk::Image image, ::vk::Format format, ::vk::ImageLayout oldLayout, ::vk::ImageLayout newLayout)
{

    device.excuteCmd([&](::vk::CommandBuffer& cmd){
        ::vk::ImageMemoryBarrier barrier;
        ::vk::ImageSubresourceRange subsourceRange;
        subsourceRange.setAspectMask(::vk::ImageAspectFlagBits::eColor)
                        .setBaseMipLevel(0)
                        .setLevelCount(imageMipLevels_)
                        .setBaseArrayLayer(0)
                        .setLayerCount(1);
        barrier.setOldLayout(oldLayout)
                .setNewLayout(newLayout)
                .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
                .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
                .setImage(image)
                .setSubresourceRange(subsourceRange);

        ::vk::PipelineStageFlags sourceStage;
        ::vk::PipelineStageFlags destinationStage;

        if (oldLayout ==::vk::ImageLayout::eUndefined && newLayout == ::vk::ImageLayout::eTransferDstOptimal) {
            barrier.setSrcAccessMask(::vk::AccessFlagBits::eNone)
                    .setDstAccessMask(::vk::AccessFlagBits::eTransferWrite);

            sourceStage = ::vk::PipelineStageFlagBits::eTopOfPipe;
            destinationStage = ::vk::PipelineStageFlagBits::eTransfer;
        } else if (oldLayout == ::vk::ImageLayout::eTransferDstOptimal && newLayout == ::vk::ImageLayout::eShaderReadOnlyOptimal) {
            
            barrier.setSrcAccessMask(::vk::AccessFlagBits::eTransferWrite)
                    .setDstAccessMask(::vk::AccessFlagBits::eShaderRead);

            sourceStage = ::vk::PipelineStageFlagBits::eTransfer;
            destinationStage = ::vk::PipelineStageFlagBits::eFragmentShader;
        } else {
            throw std::invalid_argument("unsupported layout transition!");
        }
        cmd.pipelineBarrier(sourceStage, destinationStage, ::vk::DependencyFlagBits::eByRegion, nullptr, nullptr, barrier);
    });
}

void ImageTexture::createTextureSampler(core::Device& device)
{
    ::vk::SamplerCreateInfo samplerInfo;
    samplerInfo.setMagFilter(::vk::Filter::eLinear)
                .setMinFilter(::vk::Filter::eLinear)
                .setAddressModeU(::vk::SamplerAddressMode::eRepeat)
                .setAddressModeV(::vk::SamplerAddressMode::eRepeat)
                .setAddressModeW(::vk::SamplerAddressMode::eRepeat)
                .setAnisotropyEnable(VK_TRUE)
                .setMaxAnisotropy(device.getMaxAnisotropy())
                .setBorderColor(::vk::BorderColor::eIntOpaqueBlack)
                .setUnnormalizedCoordinates(VK_FALSE)
                .setCompareEnable(VK_FALSE)
                .setCompareOp(::vk::CompareOp::eAlways)
                .setMipmapMode(::vk::SamplerMipmapMode::eLinear)
                .setMipLodBias(0.0f)
                .setMinLod(0.0f)
                .setMaxLod(imageMipLevels_);
    sampler_ = device.logicalDevice().createSampler(samplerInfo);
}

void ImageTexture::createTextureImageView(core::Device& device)
{
    imageView_ = device.createImageView(image_, format_, ::vk::ImageAspectFlagBits::eColor, imageMipLevels_);
}

void ImageTexture::generateMipmaps(core::Device& device, ::vk::Image image, int texWidth, int texHeight, uint32_t mipLevels)
{
    ::vk::FormatProperties formatProperties = device.getPhysicalDevice().getFormatProperties(::vk::Format::eR8G8B8A8Srgb);
    if(!(formatProperties.optimalTilingFeatures & ::vk::FormatFeatureFlagBits::eSampledImageFilterLinear))
    {
        throw ::std::runtime_error("texture image format does not suport linear blitting");
    }

    device.excuteCmd([&](::vk::CommandBuffer &cmd){
        ::vk::ImageMemoryBarrier barrier;
        vk::ImageSubresourceRange subresourceRange;
        barrier.setImage(image)
            .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
        subresourceRange.setAspectMask(::vk::ImageAspectFlagBits::eColor)
                        .setBaseArrayLayer(0)
                        .setLayerCount(1)
                        .setLevelCount(1);

        int mipWidth = texWidth;
        int mipHeight = texHeight;
        for(int i = 1; i < mipLevels; i++)
        {
            subresourceRange.setBaseMipLevel(i - 1);
            barrier.setOldLayout(::vk::ImageLayout::eTransferDstOptimal)
                    .setNewLayout(::vk::ImageLayout::eTransferSrcOptimal)
                    .setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
                    .setDstAccessMask(vk::AccessFlagBits::eTransferRead);
            barrier.setSubresourceRange(subresourceRange);
            cmd.pipelineBarrier(::vk::PipelineStageFlagBits::eTransfer, ::vk::PipelineStageFlagBits::eTransfer, 
                                ::vk::DependencyFlagBits::eByRegion, nullptr, nullptr, barrier);
            ::vk::ImageBlit blit;
            std::array<::vk::Offset3D, 2> srcOffsets{::vk::Offset3D{0, 0, 0}, ::vk::Offset3D{mipWidth, mipHeight, 1}};
            std::array<::vk::Offset3D, 2> dstOffsets{::vk::Offset3D{0, 0, 0}, ::vk::Offset3D{mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1? mipHeight / 2 :1, 1}};
            vk::ImageSubresourceLayers srcSubresource;
            srcSubresource.setAspectMask(vk::ImageAspectFlagBits::eColor)
                            .setMipLevel(i - 1)
                            .setBaseArrayLayer(0)
                            .setLayerCount(1);
            vk::ImageSubresourceLayers dstSubresource;
            dstSubresource.setAspectMask(vk::ImageAspectFlagBits::eColor)
                            .setMipLevel(i)
                            .setBaseArrayLayer(0)
                            .setLayerCount(1);
            blit.setSrcOffsets(srcOffsets)
                .setDstOffsets(dstOffsets)
                .setSrcSubresource(srcSubresource)
                .setDstSubresource(dstSubresource);
            cmd.blitImage(image, ::vk::ImageLayout::eTransferSrcOptimal, image, ::vk::ImageLayout::eTransferDstOptimal, blit, ::vk::Filter::eLinear);
            barrier.setOldLayout(vk::ImageLayout::eTransferSrcOptimal)
                    .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
                    .setSrcAccessMask(vk::AccessFlagBits::eTransferRead)
                    .setDstAccessMask(vk::AccessFlagBits::eShaderRead);
            cmd.pipelineBarrier(::vk::PipelineStageFlagBits::eTransfer, ::vk::PipelineStageFlagBits::eFragmentShader, 
                                    ::vk::DependencyFlagBits::eByRegion, nullptr, nullptr, barrier);
            if(mipWidth > 1)
            {
                mipWidth /= 2;
            }
            if(mipHeight > 1)
            {
                mipHeight /= 2;
            }
            
        }

        subresourceRange.setBaseMipLevel(mipLevels - 1);
        barrier.setOldLayout(vk::ImageLayout::eTransferDstOptimal)
                .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
                .setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
                .setSubresourceRange(subresourceRange)
                .setDstAccessMask(vk::AccessFlagBits::eShaderRead);

        cmd.pipelineBarrier(::vk::PipelineStageFlagBits::eTransfer, ::vk::PipelineStageFlagBits::eFragmentShader, 
                        ::vk::DependencyFlagBits::eByRegion, nullptr, nullptr, barrier);
    });

}

ImageTexture::~ImageTexture()
{
    device_.get().destroySampler(sampler_);
    device_.get().destroyImageView(imageView_);
    device_.get().destroyImage(image_);
    device_.get().freeMemory(imageMemory_);
}

}