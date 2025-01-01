#include "render_pass.hpp"
#include "vulkan_common/device.hpp"
namespace render::vulkan {
namespace {

// auto attachmentDescription(vk::Format format, vk::SampleCountFlagBits samples)
//     -> vk::AttachmentDescription {
//     vk::AttachmentDescription attach;
//     attach.setLoadOp(vk::AttachmentLoadOp::eLoad)
//         .setFormat(format)
//         .setStoreOp(vk::AttachmentStoreOp::eStore)
//         .setStencilLoadOp(vk::AttachmentLoadOp::eLoad)
//         .setStencilStoreOp(vk::AttachmentStoreOp::eStore)
//         .setInitialLayout(vk::ImageLayout::eGeneral)
//         .setFinalLayout(vk::ImageLayout::eGeneral)
//         .setSamples(samples);
//     return attach;
// }
}  // namespace
}  // namespace render::vulkan
