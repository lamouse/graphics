#include "render_pass.hpp"
#include "vulkan_common/device.hpp"
#include <boost/container/static_vector.hpp>
namespace render::vulkan {
namespace {
auto AttachmentDescription(const Device& device, surface::PixelFormat format,
                           vk::SampleCountFlagBits samples) -> vk::AttachmentDescription {
    return vk::AttachmentDescription()
        .setFormat(device.surfaceFormat(FormatType::Optimal, true, format).format)
        .setSamples(samples)
        .setLoadOp(vk::AttachmentLoadOp::eLoad)
        .setStoreOp(vk::AttachmentStoreOp::eStore)
        .setStencilLoadOp(vk::AttachmentLoadOp::eLoad)
        .setStencilStoreOp(vk::AttachmentStoreOp::eStore)
        .setInitialLayout(vk::ImageLayout::eGeneral)
        .setFinalLayout(vk::ImageLayout::eGeneral);
}

}  // namespace
RenderPassCache::RenderPassCache(const Device& device_) : device{&device_} {}
auto RenderPassCache::get(const RenderPassKey& key) -> vk::RenderPass {
    std::scoped_lock lock{mutex};
    const auto [pair, is_new] = cache.try_emplace(key);
    if (!is_new) {
        return *pair->second;
    }
    boost::container::static_vector<vk::AttachmentDescription, 9> descriptions;
    std::array<vk::AttachmentReference, 8> references{};
    u32 num_attachments{};
    u32 num_colors{};
    for (size_t index = 0; index < key.color_formats.size(); ++index) {
        const surface::PixelFormat format{key.color_formats[index]};
        const bool is_valid{format != surface::PixelFormat::Invalid};
        references[index] = vk::AttachmentReference{
            is_valid ? num_colors : VK_ATTACHMENT_UNUSED,
            vk::ImageLayout::eGeneral,
        };
        if (is_valid) {
            descriptions.push_back(AttachmentDescription(*device, format, key.samples));
            num_attachments = static_cast<u32>(index + 1);
            ++num_colors;
        }
    }
    const bool has_depth{key.depth_format != surface::PixelFormat::Invalid};
    vk::AttachmentReference depth_reference{};
    if (key.depth_format != surface::PixelFormat::Invalid) {
        depth_reference.setAttachment(num_colors).setLayout(vk::ImageLayout::eGeneral);
        descriptions.push_back(AttachmentDescription(*device, key.depth_format, key.samples));
    }
    const vk::SubpassDescription subpass =
        vk::SubpassDescription()
            .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
            .setColorAttachmentCount(num_attachments)
            .setPColorAttachments(references.data())
            .setPDepthStencilAttachment(has_depth ? &depth_reference : nullptr);
    pair->second = device->logical().createRenderPass(
        vk::RenderPassCreateInfo().setAttachments(descriptions).setSubpasses(subpass));

    return *pair->second;
}
}  // namespace render::vulkan
