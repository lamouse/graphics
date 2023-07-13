#pragma once
#include <functional>
#include <vulkan/vulkan.hpp>

#include "../core/device.hpp"
#include "image.hpp"

namespace resource::image {

class ImageTexture {
    private:
        ::vk::Sampler sampler_;
        ::vk::Image image_;
        ::vk::DeviceMemory imageMemory_;
        ::vk::ImageView imageView_;
        uint32_t imageMipLevels_;
        ::vk::Format format_;
        ::std::reference_wrapper<::vk::Device> device_;
        void loadImage();
        void transitionImageLayout(core::Device& device, ::vk::Image image, ::vk::ImageLayout oldLayout,
                                   ::vk::ImageLayout newLayout) const;
        static void generateMipmaps(core::Device& device, ::vk::Image image, int texWidth, int texHeight,
                                    uint32_t mipLevels);
        void createTextureSampler(core::Device& device);
        void createTextureImageView(core::Device& device);

    public:
        auto sampler() -> ::vk::Sampler { return sampler_; }
        auto imageView() -> ::vk::ImageView { return imageView_; }
        [[nodiscard]] auto mipLevels() const -> uint32_t { return imageMipLevels_; }
        ImageTexture(core::Device& device, Image& image, ::vk::Format format);
        auto descriptorImageInfo() -> ::vk::DescriptorImageInfo;

        ImageTexture(const ImageTexture&) = delete;
        ImageTexture(ImageTexture&&) = default;
        auto operator=(const ImageTexture&) -> ImageTexture = delete;
        auto operator=(ImageTexture&&) -> ImageTexture& = default;
        ~ImageTexture();
};

}  // namespace resource::image