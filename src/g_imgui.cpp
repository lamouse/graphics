//
// Created by ziyu on 2023/11/6:0006.
//

#include "g_imgui.hpp"

#include <spdlog/spdlog.h>

#include "core/device.hpp"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_vulkan.h"

namespace g {

namespace {

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

/**
 * @brief 初始化主要是需要自己创建一个renderpass，传入一个descriptorPool
 *
 * @param window
 * @param descriptorPool
 * @param scale
 */
void Imgui::init(GLFWwindow* window, ::vk::DescriptorPool& descriptorPool, vk::RenderPass renderPass, float scale) {
    core::Device device;
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
    //io.ConfigViewportsNoAutoMerge = true;
    //io.ConfigViewportsNoTaskBarIcon = true;
    //  Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular
    // ones.
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = .0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        //style.Colors[ImGuiCol_WindowBg] = ImVec4(.0f, .0f, .0f, 1.0f);
        // style.Colors[ImGuiCol_TitleBg] = ImVec4(.0f, .0f, 0.0f, 1.0f);
        // style.Colors[ImGuiCol_TitleBgActive] = ImVec4(.0f, .0f, 0.0f, 1.0f);
        // style.Colors[ImGuiCol_TitleBgActive] = ImVec4(.0f, .0f, 0.0f, 1.0f);
        // style.Colors[ImGuiCol_DockingPreview] = ImVec4(.0f, .0f, 0.0f, 1.0f);

    }

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForVulkan(window, true);
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = device.getVKInstance();
    init_info.PhysicalDevice = device.getPhysicalDevice();
    init_info.Device = device.logicalDevice();
    init_info.QueueFamily = device.getQueueFamilyIndices().graphicsIndex();
    init_info.Queue = device.getQueue(core::Device::DeviceQueue::graphics);
    init_info.PipelineCache = VK_NULL_HANDLE;
    init_info.DescriptorPool = descriptorPool;
    init_info.Subpass = 0;
    init_info.MinImageCount = 2;
    init_info.ImageCount = 2;
    init_info.MSAASamples = VK_SAMPLE_COUNT_8_BIT;
    init_info.Allocator = VK_NULL_HANDLE;
    init_info.CheckVkResultFn = check_vk_result;
    //init_info.UseDynamicRendering = true;
    init_info.RenderPass = renderPass;
    ImGui_ImplVulkan_Init(&init_info);
}

void Imgui::draw(ImguiDebugInfo& debugInfo, const vk::CommandBuffer& commandBuffer) {
    ImGuiIO const& io = ImGui::GetIO();
    (void)io;
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    {
        {
            ImGuiWindowFlags window_flags = 0;
            // window_flags |= ImGuiWindowFlags_NoBackground;
            // window_flags |= ImGuiWindowFlags_NoTitleBar;
            // etc.
            bool open_ptr = true;
            ImGui::SetNextWindowBgAlpha(1.0f);

            ImGui::Begin("debug window", &open_ptr, window_flags);  // Create a window called "Hello, world!" and append into it.
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


            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);

            ImGui::End();
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
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

}  // namespace g