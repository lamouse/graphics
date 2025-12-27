module;

#include <spdlog/spdlog.h>

#include <imgui.h>
#include <imgui_impl_vulkan.h>
#include "common/settings.hpp"
#include <vulkan/vulkan.hpp>

#include <vulkan/vk_enum_string_helper.h>
module render.vulkan.ImGui;
import render.vulkan.common;
import render.vulkan.present.present_frame;
import render.vulkan.present_manager;
import render.vulkan.scheduler;
import core;

namespace {
void init_imgui(float scale) {
    // 这里使用了imgui的一个分支docking
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls
    // io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;      // Enable Docking
    // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;    // Enable Multi-Viewport / Platform
    // io.DisplayFramebufferScale = ImVec2(scale, scale);
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

    ImFontConfig fontConfig;
    fontConfig.OversampleH = 2;  // 水平方向抗锯齿
    fontConfig.OversampleV = 2;  // 垂直方向抗锯齿
    io.Fonts->AddFontFromFileTTF("fonts/AlibabaPuHuiTi-3-55-Regular.otf", 18.0F, &fontConfig,
                                 io.Fonts->GetGlyphRangesChineseFull());
    ImFontConfig iconConfig;
    iconConfig.MergeMode = true;
    iconConfig.PixelSnapH = true;
    io.Fonts->AddFontFromFileTTF("fonts/MesloLGS NF Regular.ttf", 18.0F, &iconConfig);
}
}  // namespace

namespace render::vulkan {

namespace {

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
    SPDLOG_ERROR("[vulkan] Error: VkResult = {}", string_VkResult(err));
    if (err < 0) {
        // abort();
    }
}
}  // namespace

ImguiCore::ImguiCore(core::frontend::BaseWindow* window_, const Device& device_,
                     scheduler::Scheduler& scheduler_, vk::PhysicalDevice physical,
                     vk::Instance instance)
    : device(device_),
      descriptorPool(createDescriptorPool(device)),
      window(window_),
      scheduler(scheduler_),
      is_render_finish(true) {
    init_imgui(window_->getWindowSystemInfo().render_surface_scale);
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
    init_info.MinImageCount = 3;
    init_info.ImageCount = 3;
    init_info.Allocator = VK_NULL_HANDLE;
    init_info.CheckVkResultFn = check_vk_result;
    init_info.PipelineInfoMain.RenderPass = nullptr;
    constexpr auto format = VK_FORMAT_B8G8R8A8_UNORM;
    init_info.PipelineInfoMain.PipelineRenderingCreateInfo.pColorAttachmentFormats = &format;
    init_info.PipelineInfoMain.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
    init_info.PipelineInfoMain.PipelineRenderingCreateInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;

    init_info.UseDynamicRendering = true;
    init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.PipelineInfoMain.Subpass = 0;
    ImGui_ImplVulkan_LoadFunctions(
        0,
        [](const char* name, void* userData) -> PFN_vkVoidFunction {
            return vk::Instance(static_cast<VkInstance>(userData)).getProcAddr(name);
        },
        instance);
    ImGui_ImplVulkan_Init(&init_info);
}

void ImguiCore::draw(const std::function<void()>& draw_func, Frame* frame) {
    if (!draw_func) {
        return;
    }
    if (settings::values.use_present_thread.GetValue()) {
        while (!is_render_finish.load()) {
        }
        is_render_finish.exchange(false);
    }

    const vk::Extent2D extent{
        frame->width,
        frame->height,
    };
    newFrame();
    draw_func();
    ImGui::Render();
    scheduler.record([this, view = *frame->image_view, extent](vk::CommandBuffer cmdbuf) -> void {
        vk::RenderingAttachmentInfo colorAttachment =
            vk::RenderingAttachmentInfo()
                .setImageView(view)
                .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
                .setLoadOp(vk::AttachmentLoadOp::eLoad)
                .setStoreOp(vk::AttachmentStoreOp::eStore)
                .setClearValue(vk::ClearValue().setColor({0.0f, 0.0f, 0.0f, 1.0f}));
        vk::RenderingInfo renderingInfo{};
        renderingInfo.setColorAttachments(colorAttachment)
            .setLayerCount(1)
            .setRenderArea(vk::Rect2D().setExtent(extent));
        // 开始动态渲染
        cmdbuf.beginRendering(&renderingInfo);
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdbuf);
        // 结束动态渲染
        cmdbuf.endRendering();
        if (settings::values.use_present_thread.GetValue()) {
            is_render_finish.exchange(true);
        }
    });

    {
        std::scoped_lock lock(scheduler.submit_mutex_);
        ImGuiIO const& io = ImGui::GetIO();
        (void)io;
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {  // NOLINT
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }
    }
}

ImguiCore::~ImguiCore() {
    device.getLogical().waitIdle();
    ImGui_ImplVulkan_Shutdown();
    window->destroyGUI();
    ImGui::DestroyContext();
}

void ImguiCore::newFrame() {
    window->newFrame();
    ImGui_ImplVulkan_NewFrame();
    ImGui::NewFrame();
}

}  // namespace render::vulkan