//
// Created by ziyu on 2023/11/6:0006.
//

#include "vk_imgui.hpp"

#include <spdlog/spdlog.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

namespace render::vulkan {

namespace {

auto create_render_pass(const Device& device) -> RenderPass {
    ::vk::AttachmentDescription colorAttachment;
    colorAttachment.setFormat(vk::Format::eB8G8R8A8Unorm)
        .setSamples(vk::SampleCountFlagBits::e8)
        .setLoadOp(::vk::AttachmentLoadOp::eClear)
        .setStoreOp(::vk::AttachmentStoreOp::eStore)
        .setStencilLoadOp(::vk::AttachmentLoadOp::eDontCare)
        .setStencilStoreOp(::vk::AttachmentStoreOp::eDontCare)
        .setInitialLayout(::vk::ImageLayout::eUndefined)
        .setFinalLayout(::vk::ImageLayout::ePresentSrcKHR);

    ::vk::AttachmentDescription depthAttachment;
    depthAttachment.setFormat(vk::Format::eD32Sfloat)
        .setSamples(vk::SampleCountFlagBits::e8)
        .setLoadOp(::vk::AttachmentLoadOp::eClear)
        .setStoreOp(::vk::AttachmentStoreOp::eDontCare)
        .setStencilLoadOp(::vk::AttachmentLoadOp::eDontCare)
        .setStencilStoreOp(::vk::AttachmentStoreOp::eDontCare)
        .setInitialLayout(::vk::ImageLayout::eUndefined)
        .setFinalLayout(::vk::ImageLayout::eDepthStencilAttachmentOptimal);

    ::vk::AttachmentDescription colorAttachmentResolve;
    colorAttachmentResolve.setFormat(vk::Format::eB8G8R8A8Unorm)
        .setSamples(::vk::SampleCountFlagBits::e1)
        .setLoadOp(::vk::AttachmentLoadOp::eDontCare)
        .setStoreOp(::vk::AttachmentStoreOp::eStore)
        .setStencilLoadOp(::vk::AttachmentLoadOp::eDontCare)
        .setStencilStoreOp(::vk::AttachmentStoreOp::eDontCare)
        .setInitialLayout(::vk::ImageLayout::eUndefined)
        .setFinalLayout(::vk::ImageLayout::ePresentSrcKHR);
    ::std::array<::vk::AttachmentDescription, 3> attachments = {colorAttachment, depthAttachment,
                                                                colorAttachmentResolve};

    ::vk::AttachmentReference colorAttachmentRef(0, ::vk::ImageLayout::eColorAttachmentOptimal);
    ::vk::AttachmentReference depthAttachmentRef(1,
                                                 ::vk::ImageLayout::eDepthStencilAttachmentOptimal);
    ::vk::AttachmentReference colorAttachmentResolveRef(2,
                                                        ::vk::ImageLayout::eColorAttachmentOptimal);

    ::vk::SubpassDescription subpass;
    subpass.setPipelineBindPoint(::vk::PipelineBindPoint::eGraphics)
        .setColorAttachments(colorAttachmentRef)
        .setPDepthStencilAttachment(&depthAttachmentRef)
        .setResolveAttachments(colorAttachmentResolveRef);

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
    return device.logical().createRenderPass(createInfo);
}

auto createDescriptorPool(const Device& device) -> VulkanDescriptorPool {
    constexpr int count = 100;
    std::array sizes = {vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, count),
                        vk::DescriptorPoolSize(vk::DescriptorType::eSampler, count),
                        vk::DescriptorPoolSize(vk::DescriptorType::eSampledImage, count),
                        vk::DescriptorPoolSize(vk::DescriptorType::eStorageImage, count),
                        vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, count),
                        vk::DescriptorPoolSize(vk::DescriptorType::eStorageBuffer, count),
                        vk::DescriptorPoolSize(vk::DescriptorType::eUniformTexelBuffer, count),
                        vk::DescriptorPoolSize(vk::DescriptorType::eStorageTexelBuffer, count),
                        vk::DescriptorPoolSize(vk::DescriptorType::eInputAttachment, count)};
    return device.logical().createDescriptorPool(
        vk::DescriptorPoolCreateInfo()
            .setPoolSizes(sizes)
            .setFlags(::vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet)
            .setMaxSets(count));
}

void check_vk_result(VkResult err) {
    if (err == 0) {
        return;
    }
    spdlog::error("[vulkan] Error: VkResult = {}", static_cast<long>(err));
    if (err < 0) {
        abort();
    }
}
}  // namespace

Imgui::Imgui(core::frontend::BaseWindow* window_, const Device& device, vk::PhysicalDevice physical,
             vk::Instance instance, float scale)
    : renderPass(create_render_pass(device)),
      descriptorPool(createDescriptorPool(device)),
      window(window_) {
    init_debug_info();
    // 这里使用了imgui的一个分支docking
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;      // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;    // Enable Multi-Viewport / Platfor
    io.DisplayFramebufferScale = ImVec2(scale, scale);
    io.FontGlobalScale = scale;
    // io.ConfigViewportsNoAutoMerge = true;
    // io.ConfigViewportsNoTaskBarIcon = true;
    //   Setup Dear ImGui style
    ImGui::StyleColorsDark();
    // ImGui::StyleColorsLight();

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look
    // identical to regular ones.
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = .0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        // style.Colors[ImGuiCol_WindowBg] = ImVec4(.0f, .0f, .0f, 1.0f);
        //  style.Colors[ImGuiCol_TitleBg] = ImVec4(.0f, .0f, 0.0f, 1.0f);
        //  style.Colors[ImGuiCol_TitleBgActive] = ImVec4(.0f, .0f, 0.0f, 1.0f);
        //  style.Colors[ImGuiCol_TitleBgActive] = ImVec4(.0f, .0f, 0.0f, 1.0f);
        //  style.Colors[ImGuiCol_DockingPreview] = ImVec4(.0f, .0f, 0.0f, 1.0f);
    }

    // Setup Platform/Renderer backends
    window->configGUI();
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = instance;
    init_info.PhysicalDevice = physical;
    init_info.Device = device.getLogical();
    init_info.QueueFamily = device.getGraphicsFamily();
    init_info.Queue = device.getGraphicsQueue();
    init_info.PipelineCache = VK_NULL_HANDLE;
    init_info.DescriptorPool = *descriptorPool;
    init_info.Subpass = 0;
    init_info.MinImageCount = 2;
    init_info.ImageCount = 2;
    init_info.MSAASamples = VK_SAMPLE_COUNT_8_BIT;
    init_info.Allocator = VK_NULL_HANDLE;
    init_info.CheckVkResultFn = check_vk_result;
    // init_info.UseDynamicRendering = true;
    init_info.RenderPass = *renderPass;
    ImGui_ImplVulkan_Init(&init_info);
    init_debug_info();
}

void Imgui::init_debug_info() {
    debugInfo.speed = 90.0F;
    debugInfo.look_x = 2.0f;
    debugInfo.look_y = 2.0f;
    debugInfo.look_z = 2.0F;
    debugInfo.up_z = 1.f;
    debugInfo.rotate_z = 2.0;
    debugInfo.radians = 45.f;
    debugInfo.z_far = .1f;
    debugInfo.z_near = 10.f;
    debugInfo.center_x = 0;
    debugInfo.center_y = 0;
    debugInfo.center_z = 0;
}

void fps() {
    ImGuiIO const& io = ImGui::GetIO();
    (void)io;
    ImVec2 main_pos = ImGui::GetMainViewport()->Pos;
    auto main_size = ImGui::GetMainViewport()->Size;
    bool fps_open = false;
    ImGui::SetNextWindowBgAlpha(.0f);
    ImGui::SetNextWindowPos({main_pos.x + main_size.x, main_pos.y}, ImGuiCond_Always,
                            ImVec2(1.01f, 0.0f));
    ImGui::Begin("Window 1", &fps_open,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoMouseInputs | ImGuiWindowFlags_NoScrollbar |
                     ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoBackground |
                     ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::TextColored({0.0f, 1.0f, 0.0f, 1.0f}, "average %.3f ms/frame (%.1f FPS)",
                       1000.0f / io.Framerate, io.Framerate);
    ImGui::End();
}

void debug() {}

auto Imgui::get_uniform_buffer(float extentAspectRation) const -> UniformBufferObject {
    static auto startTime = ::std::chrono::high_resolution_clock::now();
    auto currentTime = ::std::chrono::high_resolution_clock::now();
    float time =
        ::std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime)
            .count();
    UniformBufferObject ubo;
    ubo.model =
        ::glm::rotate(glm::mat4(1.0f), time * glm::radians(debugInfo.speed),
                      glm::vec3(debugInfo.rotate_x, debugInfo.rotate_y, debugInfo.rotate_z));
    ubo.view = ::glm::lookAt(glm::vec3(debugInfo.look_x, debugInfo.look_y, debugInfo.look_z),
                             glm::vec3(debugInfo.center_x, debugInfo.center_y, debugInfo.center_z),
                             glm::vec3(debugInfo.up_x, debugInfo.center_y, debugInfo.up_z));
    ubo.proj = ::glm::perspective(glm::radians(debugInfo.radians), extentAspectRation,
                                  debugInfo.z_far, debugInfo.z_near);
    ubo.proj[1][1] *= -1;
    return ubo;
}

void Imgui::draw(const vk::CommandBuffer& commandBuffer) {
    ImGuiIO const& io = ImGui::GetIO();
    (void)io;
    ImGui_ImplVulkan_NewFrame();
    window->newFrame();
    ImGui::NewFrame();
    {
        {
            ImGuiWindowFlags window_flags = 0;
            // window_flags |= ImGuiWindowFlags_NoBackground;
            // window_flags |= ImGuiWindowFlags_NoTitleBar;
            // etc.
            bool open_ptr = true;
            ImGui::SetNextWindowBgAlpha(1.0f);

            ImGui::Begin(
                "debug window", &open_ptr,
                window_flags);  // Create a window called "Hello, world!" and append into it.
            float center_x = debugInfo.look_x + 0.3f;
            float center_y = debugInfo.look_y + 0.3f;
            float center_z = debugInfo.look_z + 0.3f;
            ImGui::SliderFloat("speed", &debugInfo.speed, .0f, 180.0f);
            ImGui::SliderFloat("look at x", &debugInfo.look_x, .0f, 8.f);
            ImGui::SliderFloat("look at y", &debugInfo.look_y, .0f, 8.f);
            ImGui::SliderFloat("look at z", &debugInfo.look_z, .0f, 8.f);

            ImGui::SliderFloat("center x", &debugInfo.center_x, .0f, center_x);
            ImGui::SliderFloat("center y", &debugInfo.center_y, .0f, center_y);
            ImGui::SliderFloat("center z", &debugInfo.center_z, .0f, center_z);

            ImGui::SliderFloat("up x", &debugInfo.up_x, .0f, 2.f);
            ImGui::SliderFloat("up y", &debugInfo.up_y, .0f, 2.f);
            ImGui::SliderFloat("up z", &debugInfo.up_z, .0f, 2.f);
            ImGui::SliderFloat("rotate x", &debugInfo.rotate_x, .0f, 10.f);
            ImGui::SliderFloat("rotate y", &debugInfo.rotate_y, .0f, 10.f);
            ImGui::SliderFloat("rotate z", &debugInfo.rotate_z, .1f, 10.f);

            ImGui::SliderFloat("radians z", &debugInfo.radians, 10.f, 180.f);

            ImGui::SliderFloat("z_near", &debugInfo.z_near, .1f, 10.f);
            ImGui::SliderFloat("z_far", &debugInfo.z_far, .1f, 10.f);
            ImGui::End();
            fps();
        }
    }
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
}

Imgui::~Imgui() {
    ImGui_ImplVulkan_Shutdown();
    window->destroyGUI();
    ImGui::DestroyContext();
}

}  // namespace render::vulkan