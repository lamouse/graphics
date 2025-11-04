#include "vk_graphic.hpp"
#include "blit_screen.hpp"
#include "render_core/render_vulkan/format_to_vk.hpp"
#include "render_core/render_vulkan/compute_pipeline.hpp"
#include <imgui_impl_vulkan.h>
#include <algorithm>
#include <tracy/Tracy.hpp>

using IsInstance = bool;

namespace {
auto buildVertexAttribute(const render::vulkan::Device& device,
                          std::span<render::VertexAttribute> attributes)
    -> boost::container::static_vector<vk::VertexInputAttributeDescription2EXT, 32> {
    boost::container::static_vector<vk::VertexInputAttributeDescription2EXT, 32> attrs;
    for (size_t i = 0; i < attributes.size(); i++) {
        auto attribute = attributes[i];
        attrs.push_back(
            vk::VertexInputAttributeDescription2EXT()
                .setBinding(0)
                .setLocation(attribute.location)
                .setFormat(render::vulkan::VertexFormat(device, attribute.type, attribute.size))
                .setOffset(attribute.offset));
    }

    return attrs;
}

auto buildVertexBinding(std::span<render::VertexBinding> bindings)
    -> boost::container::static_vector<vk::VertexInputBindingDescription2EXT, 32> {
    boost::container::static_vector<vk::VertexInputBindingDescription2EXT, 32> vertex_bindings;
    for (auto binding : bindings) {
        vertex_bindings.push_back(vk::VertexInputBindingDescription2EXT()
                                      .setBinding(binding.binding)
                                      .setStride(binding.stride)
                                      .setInputRate(binding.is_instance
                                                        ? vk::VertexInputRate::eInstance
                                                        : vk::VertexInputRate::eVertex)
                                      .setDivisor(binding.divisor));
    }

    return vertex_bindings;
}
}  // namespace

namespace render::vulkan {

VulkanGraphics::VulkanGraphics(core::frontend::BaseWindow* emu_window_, const Device& device_,
                               MemoryAllocator& memory_allocator_, scheduler::Scheduler& scheduler_,
                               ShaderNotify& shader_notify_)
    : device(device_),
      memory_allocator(memory_allocator_),
      scheduler(scheduler_),
      staging_pool(device, memory_allocator, scheduler),
      descriptor_pool(device, scheduler),
      guest_descriptor_queue(device, scheduler),
      compute_pass_descriptor_queue(device, scheduler),
      render_pass_cache(device),
      emu_window(emu_window_),
      texture_cache_runtime{device,
                            scheduler,
                            memory_allocator,
                            staging_pool,
                            render_pass_cache,
                            descriptor_pool,
                            compute_pass_descriptor_queue},
      texture_cache(texture_cache_runtime),
      buffer_cache_runtime(device, memory_allocator, scheduler, staging_pool,
                           guest_descriptor_queue, compute_pass_descriptor_queue),
      buffer_cache(buffer_cache_runtime),
      pipeline_cache(device, scheduler, descriptor_pool, guest_descriptor_queue, render_pass_cache,
                     buffer_cache, texture_cache, shader_notify_),
      wfi_event(device.logical().createEvent()) {}

VulkanGraphics::~VulkanGraphics() = default;

void VulkanGraphics::clean() {
    scheduler.requestRenderPass(texture_cache.getFramebuffer());
    clear();
}

void VulkanGraphics::clear() {
    auto config = emu_window->getActiveConfig();
    const vk::Extent2D render_area{static_cast<std::uint32_t>(config.extent.width),
                                   static_cast<std::uint32_t>(config.extent.height)};

    auto clear_value =
        vk::ClearValue().setColor(vk::ClearColorValue().setFloat32({1.f, 1.f, 1.f, 1.0F}));
    const vk::ClearAttachment clear_attachment = vk::ClearAttachment()
                                                     .setAspectMask(vk::ImageAspectFlagBits::eColor)
                                                     .setColorAttachment(0)
                                                     .setClearValue(clear_value);

    auto clear_depth = vk::ClearDepthStencilValue().setDepth(1).setStencil(0);
    auto clear_depth_value = vk::ClearValue().setDepthStencil(clear_depth);
    const vk::ClearAttachment depth_attachment = vk::ClearAttachment()
                                                     .setAspectMask(vk::ImageAspectFlagBits::eDepth)
                                                     .setColorAttachment(0)
                                                     .setClearValue(clear_depth_value);

    const vk::ClearRect clear_rect =
        vk::ClearRect()
            .setRect(vk::Rect2D().setOffset(vk::Offset2D().setX(0).setY(0)).setExtent(render_area))
            .setBaseArrayLayer(0)
            .setLayerCount(1);
    scheduler.record([clear_attachment, depth_attachment, clear_rect](vk::CommandBuffer cmdbuf) {
        cmdbuf.clearAttachments({clear_attachment, depth_attachment}, {clear_rect});
    });
}
void VulkanGraphics::dispatchCompute(const IComputeInstance& instance) {
    pipeline_cache.setsetCurrentShader(instance.getShaderHash());
    FlushWork();
    auto work = instance.getWorkgroupSize();

    auto* pipeline{pipeline_cache.currentComputePipeline(work)};
    if (!pipeline) {
        return;
    }
    buffer_cache.UploadComputeUniformBuffer(instance.getUBOData());
    for (const auto& mesh : instance.getMeshIds()) {
        const auto resource = modelResource[mesh];
        buffer_cache.bindComputeStorageBuffers(resource.vertex_buffer_id);
    }

    std::scoped_lock lock{texture_cache.mutex, buffer_cache.mutex};
    pipeline->Configure(scheduler, buffer_cache);
#ifdef MemoryBarrier
#undef MemoryBarrier
#endif
    static constexpr vk::MemoryBarrier READ_BARRIER =
        vk::MemoryBarrier()
            .setSrcAccessMask(vk::AccessFlagBits::eMemoryWrite)
            .setDstAccessMask(vk::AccessFlagBits::eMemoryRead);
    scheduler.requestOutsideRenderPassOperationContext();
    scheduler.record([](vk::CommandBuffer cmdbuf) {
        cmdbuf.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands,
                               vk::PipelineStageFlagBits::eComputeShader, {}, READ_BARRIER, nullptr,
                               nullptr);
    });
    scheduler.record(
        [work](vk::CommandBuffer cmdbuf) -> void { cmdbuf.dispatch(work[0], work[1], work[2]); });
}

auto VulkanGraphics::getDrawImage() -> ImTextureID {
    // 将 Vulkan 纹理绑定到 ImGui
    const auto& image_view = texture_cache.TryFindFramebufferImageView();
    if (!sampler) {
        ::vk::SamplerCreateInfo samplerInfo =
            ::vk::SamplerCreateInfo()
                .setMagFilter(::vk::Filter::eLinear)
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
                .setMipLodBias(0.0F)
                .setMinLod(0.0F)
                .setMaxLod(1);
        sampler = device.logical().CreateSampler(samplerInfo);
    }
    const auto [pair, is_new] =
        imgui_textures.try_emplace(image_view.first->Handle(shader::TextureType::Color2D));
    if (!is_new) {
        return pair->second;
    }
    ImTextureID imguiTextureID_{};

    imguiTextureID_ = (ImTextureID)ImGui_ImplVulkan_AddTexture(
        *sampler, image_view.first->Handle(shader::TextureType::Color2D), VK_IMAGE_LAYOUT_GENERAL);
    pair->second = imguiTextureID_;
    return imguiTextureID_;
    return 0;
}

void VulkanGraphics::UpdateDynamicStates() {
    UpdateViewportsState();
    UpdateScissorsState();
    UpdateDepthBias();
    UpdateBlendConstants();
    UpdateDepthBounds();
    UpdateStencilFaces();
    if (device.IsExtExtendedDynamicStateSupported()) {
        UpdateCullMode();
        UpdateDepthCompareOp();
        UpdateFrontFace();
        UpdateStencilOp();

        if (true) {  // TODO 这里临时用
            UpdateDepthBoundsTestEnable();
            UpdateDepthTestEnable();
            UpdateDepthWriteEnable();
            UpdateStencilTestEnable();
            if (device.IsExtExtendedDynamicState2Supported()) {
                UpdatePrimitiveRestartEnable();
                UpdateRasterizerDiscardEnable();
                UpdateDepthBiasEnable();
            }
            if (device.IsExtExtendedDynamicState3EnablesSupported()) {
                UpdateDepthClampEnable();
            }
        }
        if (device.IsExtExtendedDynamicState2ExtrasSupported()) {
            UpdateLogicOp();
        }
        if (device.IsExtExtendedDynamicState3Supported()) {
            UpdateBlending();
        }
    }

    if (device.IsExtVertexInputDynamicStateSupported()) {
        UpdateVertexInput();
    }
}
// 用于启用或禁用 图元重启（Primitive
// Restart）功能。图元重启功能主要用于在绘制图元（如三角形、线条等）时，
// 允许在索引缓冲区中插入一个特殊值（称为“重启索引”），以分隔不同的图元序列。
void VulkanGraphics::UpdatePrimitiveRestartEnable() {
    scheduler.record([enable = pipeline_state.primitiveRestartEnable](vk::CommandBuffer cmdbuf) {
        cmdbuf.setPrimitiveRestartEnableEXT(enable);
    });
}

// 用于启用或禁用 光栅化丢弃
void VulkanGraphics::UpdateRasterizerDiscardEnable() {
    scheduler.record([enable = pipeline_state.rasterizerDiscardEnable](vk::CommandBuffer cmdbuf) {
        cmdbuf.setRasterizerDiscardEnableEXT(enable);
    });
}

void VulkanGraphics::UpdateDepthBiasEnable() {
    const bool enable = pipeline_state.depthBiasEnable;
    scheduler.record([enable](vk::CommandBuffer cmdbuf) { cmdbuf.setDepthBiasEnableEXT(enable); });
}

void VulkanGraphics::UpdateVertexInput() {
    if (!fixedPipelineState.dynamic_vertex_input) {
        return;
    }
    auto resource = modelResource[current_modelId];
    auto attrs = vertex_attributes[resource.vertex_attribute_id];
    auto bindings = vertex_bindings[resource.vertex_binding_id];
    scheduler.record([bindings, attrs](vk::CommandBuffer cmdbuf) -> void {
        cmdbuf.setVertexInputEXT(bindings, attrs);
    });
}

void VulkanGraphics::UpdateCullMode() {
    scheduler.record([enabled = pipeline_state.cullMode](vk::CommandBuffer cmdbuf) {
        cmdbuf.setCullModeEXT(enabled ? vk::CullModeFlagBits::eBack : vk::CullModeFlagBits::eNone);
    });
}

void VulkanGraphics::UpdateDepthCompareOp() {
    scheduler.record(
        [](vk::CommandBuffer cmdbuf) { cmdbuf.setDepthCompareOpEXT(vk::CompareOp::eLessOrEqual); });
}

void VulkanGraphics::UpdateFrontFace() {
    scheduler.record(
        [](vk::CommandBuffer cmdbuf) { cmdbuf.setFrontFaceEXT(vk::FrontFace::eCounterClockwise); });
}

void VulkanGraphics::UpdateStencilOp() {
    // Front face defines the stencil op of both faces
    scheduler.record([](vk::CommandBuffer cmdbuf) {
        cmdbuf.setStencilOpEXT(vk::StencilFaceFlagBits::eFront, vk::StencilOp::eReplace,
                               vk::StencilOp::eReplace, vk::StencilOp::eReplace,
                               vk::CompareOp::eAlways);
    });
}

void VulkanGraphics::UpdateDepthBoundsTestEnable() {
    bool enabled = pipeline_state.depthBoundsTestEnable;
    if (enabled && !device.IsDepthBoundsSupported()) {
        SPDLOG_WARN("Depth bounds is enabled but not supported");
        enabled = false;
    }
    scheduler.record([enable = enabled](vk::CommandBuffer cmdbuf) {
        cmdbuf.setDepthBoundsTestEnableEXT(enable);
    });
}

void VulkanGraphics::UpdateDepthTestEnable() {
    scheduler.record([enable = pipeline_state.depthTestEnable](vk::CommandBuffer cmdbuf) {
        cmdbuf.setDepthTestEnableEXT(enable);
    });
}
/**
 * @brief 设置这个后深度信息显示正常了
 *
 */
void VulkanGraphics::UpdateDepthWriteEnable() {
    scheduler.record([enable = pipeline_state.depthWriteEnable](vk::CommandBuffer cmdbuf) {
        cmdbuf.setDepthWriteEnableEXT(enable);
    });
}

void VulkanGraphics::UpdateStencilTestEnable() {
    scheduler.record([enable = pipeline_state.stencilTestEnable](vk::CommandBuffer cmdbuf) {
        cmdbuf.setStencilTestEnableEXT(enable);
    });
}

void VulkanGraphics::UpdateDepthClampEnable() {
    scheduler.record([is_enabled = pipeline_state.depthClampEnable](vk::CommandBuffer cmdbuf) {
        cmdbuf.setDepthClampEnableEXT(is_enabled);
    });
}

void VulkanGraphics::UpdateLogicOp() {
    scheduler.record([](vk::CommandBuffer cmdbuf) { cmdbuf.setLogicOpEXT(vk::LogicOp::eAnd); });
}

void VulkanGraphics::UpdateBlending() {
    std::array<vk::ColorComponentFlags, 1> setup_masks{};
    for (auto& current : setup_masks) {
        current |= vk::ColorComponentFlagBits::eR;
        current |= vk::ColorComponentFlagBits::eG;
        current |= vk::ColorComponentFlagBits::eB;
        current |= vk::ColorComponentFlagBits::eA;
    }
    scheduler.record(
        [setup_masks](vk::CommandBuffer cmdbuf) { cmdbuf.setColorWriteMaskEXT(0, setup_masks); });
    std::array<VkBool32, 1> setup_enables{VK_FALSE};
    if (pipeline_state.colorBlendEnable) {
        setup_enables[0] = VK_TRUE;
    }
    scheduler.record([setup_enables](vk::CommandBuffer cmdbuf) {
        cmdbuf.setColorBlendEnableEXT(0, setup_enables);
    });

    std::array<vk::ColorBlendEquationEXT, 1> setup_blends{};
    for (auto& host_blend : setup_blends) {
        host_blend.srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
        host_blend.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
        host_blend.colorBlendOp = vk::BlendOp::eAdd;
        host_blend.srcAlphaBlendFactor = vk::BlendFactor::eSrcAlpha;
        host_blend.dstAlphaBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
        host_blend.alphaBlendOp = vk::BlendOp::eAdd;
    }
    scheduler.record([setup_blends](vk::CommandBuffer cmdbuf) {
        cmdbuf.setColorBlendEquationEXT(0, setup_blends);
    });
}

void VulkanGraphics::UpdateViewportsState() {
    auto view =
        vk::Viewport()
            .setX(pipeline_state.viewport.x)
            .setY(pipeline_state.viewport.y)
            .setWidth(pipeline_state.viewport.width == 0.f ? 1.f : pipeline_state.viewport.width)
            .setHeight(pipeline_state.viewport.height == 0.f ? 1.f : pipeline_state.viewport.height)
            .setMinDepth(pipeline_state.viewport.minDepth)
            .setMaxDepth(pipeline_state.viewport.maxDepth);
    scheduler.record([view](vk::CommandBuffer cmdbuf) { cmdbuf.setViewport(0, view); });
    return;
}

void VulkanGraphics::UpdateScissorsState() {
    vk::Rect2D scissor;
    scissor.offset.x = pipeline_state.scissors.x;
    scissor.offset.y = pipeline_state.scissors.y;
    scissor.extent.width =
        static_cast<u32>(pipeline_state.scissors.width != 0 ? pipeline_state.scissors.width : 1);
    scissor.extent.height =
        static_cast<u32>(pipeline_state.scissors.height != 0 ? pipeline_state.scissors.height : 1);
    scheduler.record([scissor](vk::CommandBuffer cmdbuf) { cmdbuf.setScissor(0, scissor); });
}

auto VulkanGraphics::AccelerateDisplay(const frame::FramebufferConfig& config, u32 pixel_stride)
    -> std::optional<FramebufferTextureInfo> {
    std::scoped_lock lock{texture_cache.mutex};
    const auto& image_view = texture_cache.TryFindFramebufferImageView();

    FramebufferTextureInfo info{};
    info.image = image_view.first->ImageHandle();
    info.image_view = image_view.first->RenderTarget();
    info.width = image_view.first->size.width;
    info.height = image_view.first->size.height;
    info.scaled_width = info.width;
    info.scaled_height = info.height;
    return info;
}

void VulkanGraphics::UpdateDepthBias() {
    scheduler.record([](vk::CommandBuffer cmdbuf) { cmdbuf.setDepthBias(0.0f, .0f, .0f); });
}

void VulkanGraphics::UpdateBlendConstants() {
    const std::array blend_color = {pipeline_state.blendColor.r, pipeline_state.blendColor.g,
                                    pipeline_state.blendColor.b, pipeline_state.blendColor.a};
    scheduler.record(
        [blend_color](vk::CommandBuffer cmdbuf) { cmdbuf.setBlendConstants(blend_color.data()); });
}

void VulkanGraphics::UpdateDepthBounds() {
    scheduler.record([](vk::CommandBuffer cmdbuf) { cmdbuf.setDepthBounds(0, 1); });
}

void VulkanGraphics::UpdateStencilFaces() {
    scheduler.record([](vk::CommandBuffer cmdbuf) {
        // Front face
        cmdbuf.setStencilReference(vk::StencilFaceFlagBits::eBack, 1);
    });
    scheduler.record([](vk::CommandBuffer cmdbuf) {
        // Front face
        cmdbuf.setStencilWriteMask(vk::StencilFaceFlagBits::eBack, 1);
    });

    scheduler.record([](vk::CommandBuffer cmdbuf) {
        // Front face
        cmdbuf.setStencilCompareMask(vk::StencilFaceFlagBits::eBack, 1);
    });
}

void VulkanGraphics::UpdateLineWidth() {
    scheduler.record([](vk::CommandBuffer cmdbuf) { cmdbuf.setLineWidth(1); });
}

auto VulkanGraphics::uploadModel(const graphics::IMeshData& meshData) -> MeshId {
    ModelResource resource;
    auto mesh_data = meshData.getMesh();
    resource.vertex_size = static_cast<u32>(mesh_data.size() * sizeof(float));
    ASSERT_MSG(!mesh_data.empty(), "upload empty vertex index");
    resource.vertex_buffer_id =
        buffer_cache.addVertexBuffer(mesh_data.data(), resource.vertex_size);
    resource.vertex_count = meshData.getVertexCount();
    if (!meshData.getIndices().empty()) {
        resource.indices_buffer_id =
            buffer_cache.addIndexBuffer(meshData.getIndices().data(), meshData.getIndices().size());
        resource.indices_count = meshData.getIndicesSize();
    }

    auto vertexAttribute = meshData.getVertexAttribute();
    resource.vertex_attribute_id =
        vertex_attributes.insert(buildVertexAttribute(device, std::span(vertexAttribute)));
    auto vertexBinding = meshData.getVertexBinding();
    resource.vertex_binding_id =
        vertex_bindings.insert(buildVertexBinding(std::span(vertexBinding)));
    return modelResource.insert(resource);
}

auto VulkanGraphics::uploadTexture(const ::resource::image::ITexture& texture) -> TextureId {
    return texture_cache.addTexture({.width = static_cast<u32>(texture.getWidth()),
                                     .height = static_cast<u32>(texture.getHeight())},
                                    texture.data(), texture.count());
}

void VulkanGraphics::draw(const graphics::IModelInstance& instance) {
    pipeline_state = instance.getPipelineState();
    pipeline_cache.setCurrentShader(instance.vertexShaderHash(), instance.fragmentShaderHash());
    current_modelId = instance.getMeshId();
    if (instance.getTextureId()) {
        texture_cache.setCurrentTexture(instance.getTextureId(), SamplerPreset::Linear);
    }
    if (!instance.getUBOData().empty()) {
        buffer_cache.UploadGraphicUniformBuffer(instance.getUBOData());
    }
    if (!instance.getPushConstants().empty()) {
        buffer_cache.UploadPushConstants(instance.getPushConstants());
    }
    if (instance.getMeshId()) {
        fixedPipelineState.dynamic_vertex_input.Assign(1);
    } else {
        fixedPipelineState.dynamic_vertex_input.Assign(0);
    }

    texture::FramebufferKey key;
    key.size.width = emu_window->getActiveConfig().extent.width;
    key.size.height = emu_window->getActiveConfig().extent.height;
    key.size.depth = 1;
    std::ranges::fill(key.color_formats, render::surface::PixelFormat::Invalid);
    key.color_formats.at(0) = surface::PixelFormat::B8G8R8A8_UNORM;
    key.depth_format = surface::PixelFormat::D32_FLOAT;
    texture_cache.setCurrentFrameBuffer(key);
    fixedPipelineState.topology = instance.getPrimitiveTopology();
    int instance_vertex_count = 0;
    if (instance.getVertexCount() > 0) {
        instance_vertex_count = instance.getVertexCount();
    }
    PrepareDraw([instance_vertex_count, this] -> void {
        uint32_t indices_size = 0;
        uint32_t vertexCount = instance_vertex_count;

        if (current_modelId) {
            const auto resource = modelResource[current_modelId];
            auto bindings = vertex_bindings[resource.vertex_binding_id];
            buffer_cache.BindVertexBuffers(resource.vertex_buffer_id, resource.vertex_size,
                                           bindings[0].stride);
            if (resource.indices_buffer_id) {
                IndexFormat index_format{IndexFormat::UnsignedShort};
                if (resource.indices_count > std::numeric_limits<uint16_t>::max()) {
                    index_format = IndexFormat::UnsignedInt;
                }
                buffer_cache.BindIndexBuffer(index_format, resource.indices_buffer_id);
            }
            indices_size = resource.indices_count;
            vertexCount = resource.vertex_count;
        }

        scheduler.record([indices_size, vertexCount](vk::CommandBuffer cmdbuf) -> void {
            if (indices_size > 0) {
                cmdbuf.drawIndexed(indices_size, 1, 0, 0, 0);
            } else {
                cmdbuf.draw(vertexCount, 1, 0, 0);
            }
        });
    });
}

void VulkanGraphics::TickFrame() {
    guest_descriptor_queue.TickFrame();
    staging_pool.TickFrame();
    {
        std::scoped_lock lock{texture_cache.mutex};
        texture_cache.TickFrame();
    }
    {
        std::scoped_lock lock{buffer_cache.mutex};
        buffer_cache.TickFrame();
    }
}

template <typename Func>
void VulkanGraphics::PrepareDraw(Func&& draw_func) {
    ZoneScopedN("VulkanGraphics::PrepareDraw()");
    GraphicsPipeline* const pipeline{pipeline_cache.currentGraphicsPipeline(fixedPipelineState)};
    if (!pipeline) {
        return;
    }
    std::scoped_lock lock{buffer_cache.mutex, texture_cache.mutex};
    pipeline->Configure();
    UpdateDynamicStates();
    draw_func();
    FlushWork();
}

void VulkanGraphics::FlushWork() {
    static constexpr u32 DRAWS_TO_DISPATCH = 4096;

    // Only check multiples of 8 draws
    static_assert(DRAWS_TO_DISPATCH % 8 == 0);
    if ((++draw_counter & 7U) != 7) {
        return;
    }
    if (draw_counter < DRAWS_TO_DISPATCH) {
        // Send recorded tasks to the worker thread
        scheduler.dispatchWork();
        return;
    }
    // Otherwise (every certain number of draws) flush execution.
    // This submits commands to the Vulkan driver.
    scheduler.flush();
    draw_counter = 0;
}

}  // namespace render::vulkan