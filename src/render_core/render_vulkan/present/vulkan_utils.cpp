#include "vulkan_utils.hpp"
namespace render::vulkan::present::utils {
namespace {
constexpr auto PIPELINE_COLOR_WRITE_MASK =
    vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
    vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
auto CreateWrappedPipelineImpl(const Device& device, RenderPass& renderpass, PipelineLayout& layout,
                               std::tuple<ShaderModule&, ShaderModule&> shaders,
                               vk::PipelineColorBlendAttachmentState blending) -> Pipeline {
    const std::array<vk::PipelineShaderStageCreateInfo, 2> shader_stages{{
        vk::PipelineShaderStageCreateInfo{

            {},
            vk::ShaderStageFlagBits::eVertex,
            *std::get<0>(shaders),
            "main",
            nullptr,
        },
        vk::PipelineShaderStageCreateInfo{
            {},
            vk::ShaderStageFlagBits::eFragment,
            *std::get<1>(shaders),
            "main",
            nullptr,
        },
    }};

    constexpr vk::PipelineVertexInputStateCreateInfo vertex_input_ci{
        {}, 0, nullptr, 0, nullptr,
    };

    constexpr vk::PipelineInputAssemblyStateCreateInfo input_assembly_ci{
        {},
        vk::PrimitiveTopology::eTriangleStrip,
        VK_FALSE,
    };

    constexpr vk::PipelineViewportStateCreateInfo viewport_state_ci{
        {}, 1, nullptr, 1, nullptr,
    };

    constexpr vk::PipelineRasterizationStateCreateInfo rasterization_ci{
        {},
        VK_FALSE,
        VK_FALSE,
        vk::PolygonMode::eFill,
        vk::CullModeFlagBits::eNone,
        vk::FrontFace::eClockwise,
        VK_FALSE,
        0.0f,
        0.0f,
        0.0f,
        1.0f,
    };

    constexpr vk::PipelineMultisampleStateCreateInfo multisampling_ci{
        {}, vk::SampleCountFlagBits::e1, VK_FALSE, 0.0f, nullptr, VK_FALSE, VK_FALSE,
    };

    const vk::PipelineColorBlendStateCreateInfo color_blend_ci{
        {}, VK_FALSE, vk::LogicOp::eCopy, blending, {0.0f, 0.0f, 0.0f, 0.0f},
    };

    constexpr std::array dynamic_states{vk::DynamicState::eViewport, vk::DynamicState::eScissor};

    const vk::PipelineDynamicStateCreateInfo dynamic_state_ci{
        {},
        dynamic_states,
    };

    return device.logical().createPipeline(vk::GraphicsPipelineCreateInfo{{},
                                                                          shader_stages,
                                                                          &vertex_input_ci,
                                                                          &input_assembly_ci,
                                                                          nullptr,
                                                                          &viewport_state_ci,
                                                                          &rasterization_ci,
                                                                          &multisampling_ci,
                                                                          nullptr,
                                                                          &color_blend_ci,
                                                                          &dynamic_state_ci,
                                                                          *layout,
                                                                          *renderpass,
                                                                          0,
                                                                          VK_NULL_HANDLE,
                                                                          0});
}
}  // namespace

auto CreateWrappedDescriptorSetLayout(const Device& device,
                                      std::initializer_list<vk::DescriptorType> types)
    -> DescriptorSetLayout {
    std::vector<vk::DescriptorSetLayoutBinding> bindings(types.size());
    for (size_t i = 0; i < types.size(); i++) {
        bindings[i] = vk::DescriptorSetLayoutBinding{
            static_cast<u32>(i),
            std::data(types)[i],
            1,
            vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
            nullptr,
        };
    }

    return device.logical().createDescriptorSetLayout(
        vk::DescriptorSetLayoutCreateInfo{{}, bindings});
}

auto CreateWrappedRenderPass(const Device& device, vk::Format format,
                             vk::ImageLayout initial_layout) -> RenderPass {
    const vk::AttachmentDescription attachment{
        vk::AttachmentDescriptionFlagBits::eMayAlias,
        format,
        vk::SampleCountFlagBits::e1,
        initial_layout == vk::ImageLayout::eUndefined ? vk::AttachmentLoadOp::eDontCare
                                                      : vk::AttachmentLoadOp::eLoad,
        vk::AttachmentStoreOp::eStore,
        vk::AttachmentLoadOp::eLoad,
        vk::AttachmentStoreOp::eStore,
        initial_layout,
        vk::ImageLayout::eGeneral,
    };

    constexpr vk::AttachmentReference color_attachment_ref{
        0,
        vk::ImageLayout::eGeneral,
    };

    const vk::SubpassDescription subpass_description{{},      vk::PipelineBindPoint::eGraphics,
                                                     0,       nullptr,
                                                     1,       &color_attachment_ref,
                                                     nullptr, nullptr,
                                                     0,       nullptr};

    constexpr vk::SubpassDependency dependency{
        VK_SUBPASS_EXTERNAL,
        0,
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        {},
        vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite,
        {}};

    return device.logical().createRenderPass(
        vk::RenderPassCreateInfo{{}, attachment, subpass_description, dependency});
}

PipelineLayout CreateWrappedPipelineLayout(const Device& device, DescriptorSetLayout& layout) {
    return device.logical().createPipelineLayout(vk::PipelineLayoutCreateInfo{
        {},
        1,
        layout.address(),
        0,
        nullptr,
    });
}

auto CreateWrappedPipeline(const Device& device, RenderPass& renderpass, PipelineLayout& layout,
                           std::tuple<ShaderModule&, ShaderModule&> shaders) -> Pipeline {
    constexpr vk::PipelineColorBlendAttachmentState color_blend_attachment_disabled{
        VK_FALSE,
        vk::BlendFactor::eZero,
        vk::BlendFactor::eZero,
        vk::BlendOp::eAdd,
        vk::BlendFactor::eZero,
        vk::BlendFactor::eZero,
        vk::BlendOp::eAdd,
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA};

    return CreateWrappedPipelineImpl(device, renderpass, layout, shaders,
                                     color_blend_attachment_disabled);
}

auto CreateWrappedPremultipliedBlendingPipeline(const Device& device, RenderPass& renderpass,
                                                PipelineLayout& layout,
                                                std::tuple<ShaderModule&, ShaderModule&> shaders)
    -> Pipeline {
    constexpr vk::PipelineColorBlendAttachmentState color_blend_attachment_premultiplied{
        VK_TRUE,           vk::BlendFactor::eOne,     vk::BlendFactor::eOneMinusConstantAlpha,
        vk::BlendOp::eAdd, vk::BlendFactor::eOne,     vk::BlendFactor::eZero,
        vk::BlendOp::eAdd, PIPELINE_COLOR_WRITE_MASK,
    };

    return CreateWrappedPipelineImpl(device, renderpass, layout, shaders,
                                     color_blend_attachment_premultiplied);
}

auto CreateWrappedCoverageBlendingPipeline(const Device& device, RenderPass& renderpass,
                                           PipelineLayout& layout,
                                           std::tuple<ShaderModule&, ShaderModule&> shaders)
    -> Pipeline {
    constexpr vk::PipelineColorBlendAttachmentState color_blend_attachment_coverage{
        VK_TRUE,           vk::BlendFactor::eSrc1Alpha, vk::BlendFactor::eOneMinusSrcAlpha,
        vk::BlendOp::eAdd, vk::BlendFactor::eOne,       vk::BlendFactor::eZero,
        vk::BlendOp::eAdd, PIPELINE_COLOR_WRITE_MASK,
    };

    return CreateWrappedPipelineImpl(device, renderpass, layout, shaders,
                                     color_blend_attachment_coverage);
}

}  // namespace render::vulkan::present::utils