#include "render_pass.hpp"
#include "vulkan_common/device.hpp"
#include <boost/container/static_vector.hpp>
namespace render::vulkan {
namespace {

auto createColorAttachmentDescription(auto& formats, vk::SampleCountFlagBits samples,
                                      bool need_resolvet, bool need_store)
    -> std::vector<vk::AttachmentDescription> {
    std::vector<vk::AttachmentDescription> attachments;
    for (auto format : formats) {
        if (format == vk::Format::eUndefined) {
            continue;
        }
        vk::AttachmentDescription attach;

        attach.setLoadOp(need_store ? vk::AttachmentLoadOp::eLoad : ::vk::AttachmentLoadOp::eClear)
            .setFormat(format)
            .setStoreOp(vk::AttachmentStoreOp::eStore)
            .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
            .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
            .setInitialLayout(vk::ImageLayout::eUndefined)
            .setFinalLayout(need_resolvet ? ::vk::ImageLayout::eColorAttachmentOptimal
                                          : vk::ImageLayout::ePresentSrcKHR)
            .setSamples(samples);
        attachments.push_back(attach);
    }

    return attachments;
}
auto createDepthAttachmentDescription(vk::Format format, vk::SampleCountFlagBits samples,
                                      bool need_store) -> vk::AttachmentDescription {
    vk::AttachmentDescription attach;
    attach.setLoadOp(need_store ? vk::AttachmentLoadOp::eLoad : ::vk::AttachmentLoadOp::eClear)
        .setFormat(format)
        .setStoreOp(need_store ? vk::AttachmentStoreOp::eDontCare : vk::AttachmentStoreOp::eStore)
        .setStencilLoadOp(need_store ? vk::AttachmentLoadOp::eLoad
                                     : vk::AttachmentLoadOp::eDontCare)
        .setStencilStoreOp(need_store ? vk::AttachmentStoreOp::eStore
                                      : vk::AttachmentStoreOp::eDontCare)
        .setInitialLayout(vk::ImageLayout::eUndefined)
        .setFinalLayout(::vk::ImageLayout::eDepthStencilAttachmentOptimal)
        .setSamples(samples);
    return attach;
}
auto createResolveAttachmentDescription(auto& formats, bool need_store)
    -> std::vector<vk::AttachmentDescription> {
    std::vector<vk::AttachmentDescription> attachments;
    for (auto format : formats) {
        if (format == vk::Format::eUndefined) {
            continue;
        }
        vk::AttachmentDescription attach;
        attach.setLoadOp(need_store ? vk::AttachmentLoadOp::eLoad : ::vk::AttachmentLoadOp::eClear)
            .setFormat(format)
            .setStoreOp(need_store ? vk::AttachmentStoreOp::eDontCare
                                   : vk::AttachmentStoreOp::eStore)
            .setStencilLoadOp(need_store ? vk::AttachmentLoadOp::eLoad
                                         : vk::AttachmentLoadOp::eDontCare)
            .setStencilStoreOp(need_store ? vk::AttachmentStoreOp::eStore
                                          : vk::AttachmentStoreOp::eDontCare)
            .setInitialLayout(vk::ImageLayout::eUndefined)
            .setFinalLayout(::vk::ImageLayout::ePresentSrcKHR)
            .setSamples(vk::SampleCountFlagBits::e1);
        attachments.push_back(attach);
    }

    return attachments;
}

vk::AttachmentDescription AttachmentDescription(const Device& device, surface::PixelFormat format,
                                                vk::SampleCountFlagBits samples) {
    return vk::AttachmentDescription{{},
                                     device.surfaceFormat(FormatType::Optimal, true, format).format,
                                     samples,
                                     vk::AttachmentLoadOp::eLoad,
                                     vk::AttachmentStoreOp::eStore,
                                     vk::AttachmentLoadOp::eLoad,
                                     vk::AttachmentStoreOp::eStore,
                                     vk::ImageLayout::eGeneral,
                                     vk::ImageLayout::eGeneral

    };
}

}  // namespace
RenderPassCache::RenderPassCache(const Device& device_) : device{&device_} {}
auto RenderPassCache::get(const RenderPassKey& key) -> vk::RenderPass {
    std::scoped_lock lock{mutex};
    const auto [pair, is_new] = cache.try_emplace(key);
    if (!is_new) {
        return pair->second;
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
        depth_reference = vk::AttachmentReference{
            num_colors,
            vk::ImageLayout::eGeneral,
        };
        descriptions.push_back(AttachmentDescription(*device, key.depth_format, key.samples));
    }
    const vk::SubpassDescription subpass{
        {},
        vk::PipelineBindPoint::eGraphics,
        0,
        nullptr,
        num_attachments,
        references.data(),
        nullptr,
        has_depth ? &depth_reference : nullptr,
        0,
        nullptr,
    };
    pair->second = device->getLogical().createRenderPass(vk::RenderPassCreateInfo{

        {},
        static_cast<u32>(descriptions.size()),
        descriptions.empty() ? nullptr : descriptions.data(),
        1,
        &subpass,
        0,
        nullptr,
    });
    return pair->second;
}
}  // namespace render::vulkan
