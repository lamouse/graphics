#pragma once
#include "image.hpp"
#include "../core/device.hpp"
#include <vulkan/vulkan.hpp>
#include <functional>
namespace resource::image {

class ImageTexture{
    private:
        ::vk::Sampler sampler_;
        ::vk::Image image_;
        ::vk::DeviceMemory imageMemory_;
        ::vk::ImageView imageView_;
        uint32_t imageMipLevels_;
        ::vk::Format format_;
        ::std::reference_wrapper<::vk::Device> device_;
        void loadImage();
        void transitionImageLayout(core::Device& device, ::vk::Image image, ::vk::Format format, ::vk::ImageLayout oldLayout, ::vk::ImageLayout newLayout);
        void generateMipmaps(core::Device& device, ::vk::Image image, int texWidth, int texHeight, uint32_t mipLevels);
        void createTextureSampler(core::Device& device);
        void createTextureImageView(core::Device& device);
    public:
        ::vk::Sampler sampler(){return sampler_;}
        ::vk::ImageView imageView(){return imageView_;}
        uint32_t mipLevels(){return imageMipLevels_;}
        ImageTexture(core::Device& device, Image& image, ::vk::Format format);
        ::vk::DescriptorImageInfo descriptorImageInfo();

        ImageTexture(const ImageTexture&) = delete;
        ImageTexture(ImageTexture&&) = default;
        ImageTexture operator=(const ImageTexture&) = delete;
        ImageTexture& operator=(ImageTexture&&) = default;
        ~ImageTexture();
};

}