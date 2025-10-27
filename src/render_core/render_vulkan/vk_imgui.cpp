//
// Created by ziyu on 2023/11/6:0006.
//

#include "vk_imgui.hpp"

#include <spdlog/spdlog.h>

#include "imgui.h"
#include "imgui_impl_vulkan.h"
#include "present/vulkan_utils.hpp"
#include "present_manager.hpp"
#include "scheduler.hpp"

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

ImguiCore::ImguiCore(core::frontend::BaseWindow* window_, const Device& device_,
                     scheduler::Scheduler& scheduler_, vk::PhysicalDevice physical,
                     vk::Instance instance)
    : device(device_),
      render_pass(present::utils::CreateWrappedRenderPass(device, vk::Format::eB8G8R8A8Unorm,
                                                          vk::ImageLayout::eUndefined)),
      descriptorPool(createDescriptorPool(device)),
      window(window_),
      scheduler(scheduler_) {

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
    init_info.PipelineInfoMain.RenderPass = *render_pass;
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
    if(!draw_func){
        return;
    }
    const vk::Framebuffer host_framebuffer{*frame->framebuffer};
    const vk::RenderPass renderPass(*render_pass);
    const vk::Extent2D extent{
        frame->width,
        frame->height,
    };
    newFrame();
    draw_func();
    endFrame();
    scheduler.record(
        [renderPass, host_framebuffer, extent](vk::CommandBuffer cmdbuf) -> void {
            present::utils::BeginRenderPass(cmdbuf, renderPass, host_framebuffer, extent);
            ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdbuf);
            cmdbuf.endRenderPass();
        });
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

// NOLINTNEXTLINE
void ImguiCore::endFrame() {
    ImGui::Render();
    ImGuiIO const& io = ImGui::GetIO();
    (void)io;
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {  // NOLINT
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
}

}  // namespace render::vulkan