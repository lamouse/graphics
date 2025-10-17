//
// Created by ziyu on 2023/11/6:0006.
//

#include "vk_imgui.hpp"

#include <spdlog/spdlog.h>

#include "imgui.h"
#include "imgui_impl_vulkan.h"
#include "present/vulkan_utils.hpp"

#include <vulkan/vk_enum_string_helper.h>

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

ImguiCore::ImguiCore(core::frontend::BaseWindow* window_, const Device& device,
                     vk::PhysicalDevice physical, vk::Instance instance, float scale)
    : render_pass(present::utils::CreateWrappedRenderPass(device, vk::Format::eB8G8R8A8Unorm,
                                                          vk::ImageLayout::eUndefined)),
      descriptorPool(createDescriptorPool(device)),
      window(window_) {
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
    init_info.MinImageCount = 3;
    init_info.ImageCount = 3;
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.Allocator = VK_NULL_HANDLE;
    init_info.CheckVkResultFn = check_vk_result;
    // init_info.UseDynamicRendering = true;
    init_info.RenderPass = *render_pass;
    ImGui_ImplVulkan_LoadFunctions(0,    [](const char* name, void* userData) -> PFN_vkVoidFunction {
        return vk::Instance(static_cast<VkInstance>(userData)).getProcAddr(name);
    },
    instance);
    ImGui_ImplVulkan_Init(&init_info);
    ImFontConfig fontConfig;
    fontConfig.OversampleH = 2;  // 水平方向抗锯齿
    fontConfig.OversampleV = 2;  // 垂直方向抗锯齿
    io.Fonts->AddFontFromFileTTF("fronts/AlibabaPuHuiTi-3-55-Regular.otf", 18.0F, &fontConfig,
                                 io.Fonts->GetGlyphRangesChineseFull());
    ImFontConfig iconConfig;
    iconConfig.MergeMode = true;
    iconConfig.PixelSnapH = true;
    io.Fonts->AddFontFromFileTTF("fronts/MesloLGS NF Regular.ttf", 18.0F, &iconConfig);
}

void ImguiCore::imgui_predraw() {
    {
        {
            bool show = false;
            ImGui::ShowDemoWindow(&show);
        }
    }
    ImGui::Render();
}
void ImguiCore::draw(const vk::CommandBuffer& commandBuffer) {
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
}

ImguiCore::~ImguiCore() {
    ImGui_ImplVulkan_Shutdown();
    window->destroyGUI();
    ImGui::DestroyContext();
}

void ImguiCore::newFrame() {
    window->newFrame();
    ImGui_ImplVulkan_NewFrame();
    ImGui::NewFrame();
}

//NOLINTNEXTLINE
void ImguiCore::endFrame() {
    ImGuiIO const& io = ImGui::GetIO();
    (void)io;
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {//NOLINT
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
}

}  // namespace render::vulkan