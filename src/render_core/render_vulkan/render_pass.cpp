#include "render_pass.hpp"
#include "vulkan_common/device.hpp"
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
}  // namespace
RenderPassCache::RenderPassCache(const Device& device_) : device{&device_} {}
auto RenderPassCache::get(const RenderPassKey& key) -> vk::RenderPass {
    std::scoped_lock lock{mutex};
    const auto [pair, is_new] = cache.try_emplace(key);
    if (!is_new) {
        return pair->second;
    }
    std::vector<vk::AttachmentDescription> attachments;
    auto color_attachments = createColorAttachmentDescription(key.color_formats, key.samples,
                                                              key.need_resolvet, key.need_store);
    ::vk::SubpassDescription subpass;
    subpass.setPipelineBindPoint(::vk::PipelineBindPoint::eGraphics);

    attachments.insert(attachments.end(), color_attachments.begin(), color_attachments.end());
    uint32_t attachment = 0;
    ::vk::AttachmentReference colorAttachmentRef(attachment,
                                                 ::vk::ImageLayout::eColorAttachmentOptimal);
    subpass.setColorAttachments(colorAttachmentRef);
    if (key.depth_format != vk::Format::eUndefined) {
        attachment++;
        auto depth_attachment =
            createDepthAttachmentDescription(key.depth_format, key.samples, key.save_depth);
        attachments.push_back(depth_attachment);
        ::vk::AttachmentReference depthAttachmentRef(
            attachment, ::vk::ImageLayout::eDepthStencilAttachmentOptimal);
        subpass.setPDepthStencilAttachment(&depthAttachmentRef);
    }
    if (key.need_resolvet) {
        attachment++;
        auto resolve_attachment =
            createResolveAttachmentDescription(key.color_formats, key.need_store);
        attachments.insert(attachments.end(), resolve_attachment.begin(), resolve_attachment.end());
        ::vk::AttachmentReference colorAttachmentResolveRef(
            attachment, ::vk::ImageLayout::eColorAttachmentOptimal);
        subpass.setPResolveAttachments(&colorAttachmentResolveRef);
    }

    ::vk::SubpassDependency subpassDependency;
    subpassDependency.setSrcSubpass(VK_SUBPASS_EXTERNAL)
        .setDstSubpass(0)
        .setSrcAccessMask(::vk::AccessFlagBits::eNone)
        .setSrcStageMask(::vk::PipelineStageFlagBits::eColorAttachmentOutput |
                         ::vk::PipelineStageFlagBits::eEarlyFragmentTests)
        .setDstAccessMask(::vk::AccessFlagBits::eColorAttachmentWrite |
                          ::vk::AccessFlagBits::eDepthStencilAttachmentWrite)
        .setDstStageMask(::vk::PipelineStageFlagBits::eColorAttachmentOutput |
                         ::vk::PipelineStageFlagBits::eEarlyFragmentTests);

    ::vk::RenderPassCreateInfo createInfo;
    createInfo.setAttachments(attachments).setSubpasses(subpass).setDependencies(subpassDependency);
    auto renderPass = device->getLogical().createRenderPass(createInfo);
    cache[key] = renderPass;
    return renderPass;
}
}  // namespace render::vulkan
