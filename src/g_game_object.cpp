#include "g_game_object.hpp"
#include "g_defines.hpp"
#include "g_device.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <stdint.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
namespace g
{


GameObject::~GameObject()
{
    if(imageLoaded)
    {
        auto& device = Device::getInstance().getVKDevice();
        device.destroySampler(textureSampler);
        device.destroyImageView(textureImageView);
        device.destroyImage(textureImage);
        device.freeMemory(textureImageMemory);
    }

}


void GameObject::loadImage()
{
    if(imageLoaded)
    {
        return;
    }
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load((image_path + "viking_room.png").c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    ::vk::DeviceSize imageSize = texWidth * texHeight * 4;
    imageMipLevels = static_cast<uint32_t>(::std::floor(::std::log2(::std::max(texWidth, texHeight)))) + 1;
    if (!pixels) {
        throw std::runtime_error("failed to load texture image!");
    }

    auto& device = Device::getInstance();
    ::vk::Buffer stagingBuffer;
    ::vk::DeviceMemory stagingBufferMemory;

    device.createBuffer(imageSize, ::vk::BufferUsageFlagBits::eTransferSrc, 
                        ::vk::MemoryPropertyFlagBits::eHostVisible | ::vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer, stagingBufferMemory);

    void* data = device.getVKDevice().mapMemory(stagingBufferMemory, 0, imageSize);
    ::memcpy(data, pixels, imageSize);
    device.getVKDevice().unmapMemory(stagingBufferMemory);
    stbi_image_free(pixels);
    device.createImage(texWidth, texHeight, imageMipLevels, ::vk::Format::eR8G8B8A8Srgb, ::vk::SampleCountFlagBits::e1, ::vk::ImageTiling::eOptimal, 
                    ::vk::ImageUsageFlagBits::eTransferSrc|::vk::ImageUsageFlagBits::eTransferDst |::vk::ImageUsageFlagBits::eSampled, 
                    ::vk::MemoryPropertyFlagBits::eDeviceLocal, textureImage, textureImageMemory);

    transitionImageLayout(textureImage, ::vk::Format::eR8G8B8A8Srgb, ::vk::ImageLayout::eUndefined, ::vk::ImageLayout::eTransferDstOptimal);

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
                .setImageExtent({static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight), 1});
        cmdBuf.copyBufferToImage(stagingBuffer, textureImage, ::vk::ImageLayout::eTransferDstOptimal, region);
    });

    transitionImageLayout(textureImage, ::vk::Format::eR8G8B8A8Srgb, ::vk::ImageLayout::eUndefined, ::vk::ImageLayout::eTransferDstOptimal);
    device.getVKDevice().destroyBuffer(stagingBuffer);
    device.getVKDevice().freeMemory(stagingBufferMemory);
    generateMipmaps(textureImage, texWidth, texHeight,  imageMipLevels);
    createTextureImageView();
    createTextureSampler();
    imageLoaded = true;
}

void GameObject::transitionImageLayout(::vk::Image image, ::vk::Format format, ::vk::ImageLayout oldLayout, ::vk::ImageLayout newLayout)
{

    Device::getInstance().excuteCmd([&](::vk::CommandBuffer& cmd){
        ::vk::ImageMemoryBarrier barrier;
        ::vk::ImageSubresourceRange subsourceRange;
        subsourceRange.setAspectMask(::vk::ImageAspectFlagBits::eColor)
                        .setBaseMipLevel(0)
                        .setLevelCount(imageMipLevels)
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

void GameObject::createTextureImageView()
{
    textureImageView = Device::getInstance().createImageView(textureImage, ::vk::Format::eR8G8B8A8Srgb, ::vk::ImageAspectFlagBits::eColor, imageMipLevels);
}

void GameObject::createTextureSampler()
{
    ::vk::SamplerCreateInfo samplerInfo;
    samplerInfo.setMagFilter(::vk::Filter::eLinear)
                .setMinFilter(::vk::Filter::eLinear)
                .setAddressModeU(::vk::SamplerAddressMode::eRepeat)
                .setAddressModeV(::vk::SamplerAddressMode::eRepeat)
                .setAddressModeW(::vk::SamplerAddressMode::eRepeat)
                .setAnisotropyEnable(VK_TRUE)
                .setMaxAnisotropy(Device::getInstance().getMaxAnisotropy())
                .setBorderColor(::vk::BorderColor::eIntOpaqueBlack)
                .setUnnormalizedCoordinates(VK_FALSE)
                .setCompareEnable(VK_FALSE)
                .setCompareOp(::vk::CompareOp::eAlways)
                .setMipmapMode(::vk::SamplerMipmapMode::eLinear)
                .setMipLodBias(0.0f)
                .setMinLod(0.0f)
                .setMaxLod(imageMipLevels);
    textureSampler = Device::getInstance().getVKDevice().createSampler(samplerInfo);
}


void GameObject::generateMipmaps(::vk::Image image, int texWidth, int texHeight, uint32_t mipLevels)
{

    auto& device = Device::getInstance();
    ::vk::FormatProperties formatProperties = device.getPhysicalDevice().getFormatProperties(::vk::Format::eR8G8B8A8Srgb);
    if(!(formatProperties.optimalTilingFeatures & ::vk::FormatFeatureFlagBits::eSampledImageFilterLinear))
    {
        throw ::std::runtime_error("texture image format does not suport linear blitting");
    }

    Device::getInstance().excuteCmd([&](::vk::CommandBuffer &cmd){
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


}