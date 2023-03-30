#include "g_game_object.hpp"
#include "g_defines.hpp"
#include "g_device.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
namespace g
{


GameObject::~GameObject()
{
    if(imageLoaded)
    {
        Device::getInstance().getVKDevice().destroySampler(textureSampler);
        Device::getInstance().getVKDevice().destroyImageView(textureImageView);
        Device::getInstance().getVKDevice().destroyImage(textureImage);
        Device::getInstance().getVKDevice().freeMemory(textureImageMemory);
    }

}


void GameObject::loadImage()
{
    if(imageLoaded)
    {
        return;
    }
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load((image_path + "p1.jpg").c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    ::vk::DeviceSize imageSize = texWidth * texHeight * 4;

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
    device.createImage(texWidth, texHeight, ::vk::Format::eB8G8R8A8Srgb, ::vk::ImageTiling::eOptimal, 
                    ::vk::ImageUsageFlagBits::eTransferDst |::vk::ImageUsageFlagBits::eSampled, ::vk::MemoryPropertyFlagBits::eDeviceLocal,
                            textureImage, textureImageMemory);

    transitionImageLayout(textureImage, ::vk::Format::eB8G8R8A8Srgb, ::vk::ImageLayout::eUndefined, ::vk::ImageLayout::eTransferDstOptimal);

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

    transitionImageLayout(textureImage, ::vk::Format::eB8G8R8A8Srgb, ::vk::ImageLayout::eTransferDstOptimal, ::vk::ImageLayout::eShaderReadOnlyOptimal);
    device.getVKDevice().destroyBuffer(stagingBuffer);
    device.getVKDevice().freeMemory(stagingBufferMemory);
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
                        .setLevelCount(1)
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
    textureImageView = Device::getInstance().createImageView(textureImage, ::vk::Format::eB8G8R8A8Srgb);
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
                .setMaxLod(0.0f);
    textureSampler = Device::getInstance().getVKDevice().createSampler(samplerInfo);
}

}