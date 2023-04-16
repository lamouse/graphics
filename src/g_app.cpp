#include "g_app.hpp"
#include "g_pipeline.hpp"
#include "g_render_system.hpp"
#include "g_defines.hpp"
#include "g_context.hpp"
#include "imgui/gui.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include <unordered_map>
namespace g{

void init_imgui(GLFWwindow* window, VkFormat format, ::vk::DescriptorPool descriptorPool);
void draw_imgui();

static ::VkRenderPass renderPass;


static void check_vk_result(VkResult err)
{
    if (err == 0)
        return;
    fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
    if (err < 0)
        abort();
}

void App::run(){

    RenderProcesser render(Context::Instance().device());
    RenderSystem renderSystem(render.getRenderPass());
    renderSystem.createUniformBuffers(2);
    ::std::string s(image_path + "viking_room.png");
    resource::image::Image img(s);
    resource::image::ImageTexture imageTexture{Context::Instance().device(), img, DEFAULT_FORMAT};
    init_imgui(window(), VK_FORMAT_B8G8R8A8_UNORM, renderSystem.getDescriptorPool());

    while (!window.shuldClose()){
        glfwPollEvents();
        if(render.beginFrame())
        {
            render.beginSwapchainRenderPass();
            renderSystem.renderGameObject(gameObjects, render.getCurrentFrameIndex(), render.getCurrentCommadBuffer(), render.extentAspectRation(), imageTexture);
            render.endSwapchainRenderPass();
            render.endFrame();
        }

        draw_imgui();

    }

    Context::Instance().device().logicalDevice().waitIdle();
}

std::unique_ptr<Model> createCubeModel(::glm::vec3 offset) {


    ::tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, (models_path + "viking_room.obj") .c_str())) {
        throw std::runtime_error(warn + err);
    }
    std::vector<Model::Vertex> vertices;
    std::vector<uint16_t> indices;
   
   ::std::unordered_map<Model::Vertex, uint32_t> uniqueVertices{};

        for (const auto& shape : shapes) {
            for (const auto& index : shape.mesh.indices) {
                Model::Vertex vertex{};

                vertex.position = {
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]
                };

                vertex.texCoord = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
                };

                vertex.color = {1.0f, 1.0f, 1.0f};

                if (uniqueVertices.count(vertex) == 0) {
                    uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                    vertices.push_back(vertex);
                }

                indices.push_back(uniqueVertices[vertex]);
            }
        }
     
  return std::make_unique<Model>(vertices, indices, Context::Instance().device());
}

void App::loadGameObjects()
{
    ::std::shared_ptr<Model> model = createCubeModel({.0f, .0f, .0f});
    auto cube = GameObject::createGameObject();
    cube.model = model;
    gameObjects.push_back(std::move(cube));

}

/**
 * @brief 初始化主要是需要自己创建一个renderpass，传入一个descriptorPool
 * 
 * @param window 
 * @param format 
 * @param descriptorPool 
 */
void init_imgui(GLFWwindow* window, VkFormat format, ::vk::DescriptorPool descriptorPool)
{
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = format;
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

    auto& device = Context::Instance().device();

    vkCreateRenderPass(device.logicalDevice(), &renderPassCreateInfo, nullptr, &renderPass);

    //这里使用了imgui的一个分支docking
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;        // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platfor

    // Setup Dear ImGui style
    //ImGui::StyleColorsDark();
    ImGui::StyleColorsLight();

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

     // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForVulkan(window, true);
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = device.getVKInstance();
    init_info.PhysicalDevice = device.getPhysicalDevice();
    init_info.Device = device.logicalDevice();
    init_info.QueueFamily = device.queueFamilyIndices.graphicsQueue.value();
    init_info.Queue = device.getGraphicsQueue();
    init_info.PipelineCache = VK_NULL_HANDLE;
    init_info.DescriptorPool = descriptorPool;
    init_info.Subpass = 0;
    init_info.MinImageCount = 2;
    init_info.ImageCount = 2;
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.Allocator = VK_NULL_HANDLE;
    init_info.CheckVkResultFn = check_vk_result;
    ImGui_ImplVulkan_Init(&init_info, renderPass);
    // Upload Fonts
    {
        // Use any command queue
        device.excuteCmd(ImGui_ImplVulkan_CreateFontsTexture);
        ImGui_ImplVulkan_DestroyFontUploadObjects();
    }

}

void draw_imgui()
{
    ImGuiIO& io = ImGui::GetIO();   
    (void)io;
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    static bool show_demo_window = false;
    static bool show_another_window = false;
    const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkPos.x + 650, main_viewport->WorkPos.y + 20), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(550, 680), ImGuiCond_FirstUseEver);
    {
        if(show_demo_window)
        {
            ImGui::ShowDemoWindow(&show_demo_window);
        }
        ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
        {
            static float f = 0.0f;
            static int counter = 0;

            ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.
        
            ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
            ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
            ImGui::Checkbox("Another Window", &show_another_window);

            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
            ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

            if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
                counter++;
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            ImGui::End();
        }

        if (show_another_window)
        {
            ImGui::Begin("Another Window");   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
            ImGui::Text("Hello from another window!");
            if (ImGui::Button("Close Me"))
                show_another_window = false;
            ImGui::End();
        }
    }
    ImGui::Render();

    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
}

App::App()
{
    loadGameObjects();
}

App::~App()
{
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    Context::Instance().device().logicalDevice().destroyRenderPass(renderPass);
}
}