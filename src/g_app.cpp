#include "g_app.hpp"

#include "core/buffer.hpp"
#include "g_defines.hpp"
#include "g_render.hpp"
#include "g_render_system.hpp"

// imgui begin
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_vulkan.h"
#include "resource/image_texture.hpp"
// imgui end
#include <spdlog/spdlog.h>

#include <chrono>

namespace g {

struct ImguiDebugInfo {
        float speed;
        float look_x;
        float look_y;
        float look_z;

        float center_x;
        float center_y;
        float center_z;

        float up_x;
        float up_y;
        float up_z;

        float rotate_x;
        float rotate_y;
        float rotate_z;
        float radians;

        float z_far;
        float z_near;
};

void init_imgui(GLFWwindow* window, core::Device& device, ::vk::DescriptorPool& descriptorPool, float scale = 1.0f);
void draw_imgui(ImguiDebugInfo& debugInfo);

namespace {

::VkRenderPass imguiRenderPass;
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

void App::run() {
    auto setLayout =
        DescriptorSetLayout::Builder()
            .addBinding(0, ::vk::DescriptorType::eUniformBuffer, ::vk::ShaderStageFlagBits::eAllGraphics)
            .addBinding(1, ::vk::DescriptorType::eCombinedImageSampler, ::vk::ShaderStageFlagBits::eAllGraphics)
            .build(device_);

    ::std::vector<::std::unique_ptr<core::Buffer>> uboBuffers(2);
    for (auto& uboBuffer : uboBuffers) {
        uboBuffer = ::std::make_unique<core::Buffer>(device_, sizeof(UniformBufferObject), 1,
                                                     ::vk::BufferUsageFlagBits::eUniformBuffer,
                                                     ::vk::MemoryPropertyFlagBits::eHostVisible);
        uboBuffer->map();
    }
    ::std::string s(image_path + "viking_room.png");
    resource::image::Image img(s);
    resource::image::ImageTexture imageTexture{device_, img, DEFAULT_FORMAT};

    ::std::vector<::vk::DescriptorSet> descriptorSets(2);
    for (::std::vector<::vk::DescriptorSet>::size_type i = 0; i < descriptorSets.size(); i++) {
        auto bufferInfo = uboBuffers[i]->descriptorInfo();
        DescriptorWriter(*setLayout, *descriptorPool_)
            .writeBuffer(0, bufferInfo)
            .writeImage(1, imageTexture.descriptorImageInfo())
            .build(descriptorSets[i]);
    }

    RenderProcesser render(device_);
    RenderSystem renderSystem(device_, render, setLayout->getDescriptorSetLayout());

    init_imgui(window(), device_, descriptorPool_->getDescriptorPool(), window.getScale());
    static auto startTime = ::std::chrono::high_resolution_clock::now();
    Camera camera{};
    ImguiDebugInfo debugInfo{};
    debugInfo.speed = 90.0F;
    debugInfo.look_x = 2.0f;
    debugInfo.look_y = 2.0f;
    debugInfo.look_z = 2.0F;
    debugInfo.up_z = 1.f;
    debugInfo.rotate_z = 2.0;
    debugInfo.radians = 45.f;
    debugInfo.z_far = .1f;
    debugInfo.z_near = 10.f;
    while (!window.shouldClose()) {
        draw_imgui(debugInfo);

        auto currentTime = ::std::chrono::high_resolution_clock::now();
        float time = ::std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
        UniformBufferObject ubo{};
        ubo.model = ::glm::rotate(glm::mat4(1.0f), time * glm::radians(debugInfo.speed),
                                  glm::vec3(debugInfo.rotate_x, debugInfo.rotate_y, debugInfo.rotate_z));
        ubo.view = ::glm::lookAt(glm::vec3(debugInfo.look_x, debugInfo.look_y, debugInfo.look_z),
                                 glm::vec3(debugInfo.center_x, debugInfo.center_y, debugInfo.center_z),
                                 glm::vec3(debugInfo.up_x, debugInfo.center_y, debugInfo.up_z));
        ubo.proj = ::glm::perspective(glm::radians(debugInfo.radians), render.extentAspectRation(), debugInfo.z_far,
                                      debugInfo.z_near);
        ubo.proj[1][1] *= -1;

        glfwPollEvents();
        if (render.beginFrame()) {
            uboBuffers[render.getCurrentFrameIndex()]->writeToBuffer(&ubo);
            uboBuffers[render.getCurrentFrameIndex()]->flush();

            FrameInfo frameInfo{render.getCurrentFrameIndex(),
                                time,
                                render.getCurrentCommadBuffer(),
                                camera,
                                descriptorSets[render.getCurrentFrameIndex()],
                                gameObjects};

            render.beginSwapchainRenderPass();
            renderSystem.render(frameInfo);
            render.endSwapchainRenderPass();
            render.endFrame();
        }
    }

    device_.logicalDevice().waitIdle();
}

void App::loadGameObjects() {
    auto cube = GameObject::createGameObject();
    cube.model = Model::createFromFile("models/viking_room.obj", device_);
    gameObjects.emplace(cube.getId(), ::std::move(cube));
}

/**
 * @brief 初始化主要是需要自己创建一个renderpass，传入一个descriptorPool
 *
 * @param window
 * @param descriptorPool
 * @param scale
 */
void init_imgui(GLFWwindow* window, core::Device& device, ::vk::DescriptorPool& descriptorPool, float scale) {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = VK_FORMAT_B8G8R8A8_UNORM;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorReference{};
    colorReference.attachment = 0;
    colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpassDescription{};
    subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.colorAttachmentCount = 1;
    subpassDescription.pColorAttachments = &colorReference;

    VkSubpassDependency supassDependency{};
    supassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    supassDependency.dstSubpass = 0;
    supassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    supassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    supassDependency.srcAccessMask = 0;
    supassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassCreateInfo{};
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.attachmentCount = 1;
    renderPassCreateInfo.pAttachments = &colorAttachment;
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpassDescription;
    renderPassCreateInfo.dependencyCount = 1;
    renderPassCreateInfo.pDependencies = &supassDependency;

    vkCreateRenderPass(device.logicalDevice(), &renderPassCreateInfo, nullptr, &imguiRenderPass);

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
    io.ConfigViewportsNoAutoMerge = true;
    // Setup Dear ImGui style
    // ImGui::StyleColorsDark();
    ImGui::StyleColorsLight();

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular
    // ones.
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForVulkan(window, true);
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = device.getVKInstance();
    init_info.PhysicalDevice = device.getPhysicalDevice();
    init_info.Device = device.logicalDevice();
    init_info.QueueFamily = device.queueFamilyIndices.graphicsQueue;
    init_info.Queue = device.getGraphicsQueue();
    init_info.PipelineCache = VK_NULL_HANDLE;
    init_info.DescriptorPool = descriptorPool;
    init_info.Subpass = 0;
    init_info.MinImageCount = 2;
    init_info.ImageCount = 2;
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.Allocator = VK_NULL_HANDLE;
    init_info.CheckVkResultFn = check_vk_result;
    ImGui_ImplVulkan_Init(&init_info, imguiRenderPass);
    // Upload Fonts
    {
        // Use any command queue
        device.executeCmd(ImGui_ImplVulkan_CreateFontsTexture);
        ImGui_ImplVulkan_DestroyFontUploadObjects();
    }
}

void draw_imgui(ImguiDebugInfo& debugInfo) {
    auto* mainviewport = ImGui::GetMainViewport();
    mainviewport->Flags |= ImGuiViewportFlags_NoRendererClear;
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    static bool show_demo_window = false;
    static bool show_another_window = false;
    const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkPos.x + 650, main_viewport->WorkPos.y + 20),
                            ImGuiCond_FirstUseEver);
    {
        if (show_demo_window) {
            ImGui::ShowDemoWindow(&show_demo_window);
        }
        {
            static int counter = 0;

            ImGui::Begin("debug window");  // Create a window called "Hello, world!" and append into it.

            ImGui::Text("This is some useful text.");           // Display some text (you can use a format strings too)
            ImGui::Checkbox("Demo Window", &show_demo_window);  // Edit bools storing our window open/close state
            ImGui::Checkbox("Another Window", &show_another_window);
            float cenetr_x = debugInfo.look_x + 0.3f;
            float cenetr_y = debugInfo.look_y + 0.3f;
            float cenetr_z = debugInfo.look_z + 0.3f;
            ImGui::SliderFloat("speed", &debugInfo.speed, .0f, 180.0f);
            ImGui::SliderFloat("look at x", &debugInfo.look_x, .0f, 8.f);
            ImGui::SliderFloat("look at y", &debugInfo.look_y, .0f, 8.f);
            ImGui::SliderFloat("look at z", &debugInfo.look_z, .0f, 8.f);

            ImGui::SliderFloat("center x", &debugInfo.center_x, .0f, cenetr_x);
            ImGui::SliderFloat("center y", &debugInfo.center_y, .0f, cenetr_y);
            ImGui::SliderFloat("center z", &debugInfo.center_z, .0f, cenetr_z);

            ImGui::SliderFloat("up x", &debugInfo.up_x, .0f, 2.f);
            ImGui::SliderFloat("up y", &debugInfo.up_y, .0f, 2.f);
            ImGui::SliderFloat("up z", &debugInfo.up_z, .0f, 2.f);

            ImGui::SliderFloat("rotate x", &debugInfo.rotate_x, .0f, 10.f);
            ImGui::SliderFloat("rotate y", &debugInfo.rotate_y, .0f, 10.f);
            ImGui::SliderFloat("rotate z", &debugInfo.rotate_z, .1f, 10.f);

            ImGui::SliderFloat("radians z", &debugInfo.radians, 10.f, 180.f);

            ImGui::SliderFloat("z_near", &debugInfo.z_near, .1f, 10.f);
            ImGui::SliderFloat("z_far", &debugInfo.z_far, .1f, 10.f);

            if (ImGui::Button(
                    "Button")) {  // Buttons return true when clicked (most widgets return true when edited/activated)
                counter++;
            }
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            ImGui::End();
        }

        if (show_another_window) {
            ImGui::Begin("Another Window");  // Pass a pointer to our bool variable (the window will have a closing
                                             // button that will clear the bool when clicked)
            ImGui::Text("Hello from another window!");
            if (ImGui::Button("Close Me")) {
                show_another_window = false;
            }
            ImGui::End();
        }
    }
    ImGui::Render();

    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
}

App::App() {
    constexpr unsigned count = 1000;
    descriptorPool_ = DescriptorPool::Builder()
                          .setMaxSets(count)
                          .addPoolSize(::vk::DescriptorType::eUniformBuffer, count)
                          .addPoolSize(::vk::DescriptorType::eSampler, count)
                          .addPoolSize(::vk::DescriptorType::eCombinedImageSampler, count)
                          .addPoolSize(::vk::DescriptorType::eStorageImage, count)
                          .addPoolSize(::vk::DescriptorType::eSampledImage, count)
                          .addPoolSize(::vk::DescriptorType::eUniformTexelBuffer, count)
                          .addPoolSize(::vk::DescriptorType::eStorageTexelBuffer, count)
                          .addPoolSize(::vk::DescriptorType::eStorageBuffer, count)
                          .addPoolSize(::vk::DescriptorType::eStorageBufferDynamic, count)
                          .addPoolSize(::vk::DescriptorType::eUniformBufferDynamic, count)
                          .addPoolSize(::vk::DescriptorType::eInputAttachment, count)
                          .build(device_);
    loadGameObjects();
}

App::~App() {
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    device_.logicalDevice().destroyRenderPass(imguiRenderPass);
}
}  // namespace g