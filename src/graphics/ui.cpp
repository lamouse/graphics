#include "imgui.h"
#include "ui.hpp"
#include "common/settings.hpp"
#include <ranges>
#include <chrono>
namespace {
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
    ImGui::TextColored({0.0f, 1.0f, 0.0f, 1.0f}, "FPS: %.1f ", io.Framerate);
    ImGui::End();
}
ImVec2 calculateAspectRatioSize(ImVec2 availableSize, float aspectRatio) {
    float targetWidth = availableSize.x;
    float targetHeight = availableSize.x / aspectRatio;

    // 如果高度超出可用区域，则按高度缩放
    if (targetHeight > availableSize.y) {
        targetHeight = availableSize.y;
        targetWidth = availableSize.y * aspectRatio;
    }

    return ImVec2(targetWidth, targetHeight);
}

static void HelpMarker(const char* desc) {
    ImGui::TextDisabled("(?)");
    if (ImGui::BeginItemTooltip()) {
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

void vsync_setting() {
    auto canon = settings::enums::EnumMetadata<settings::enums::VSyncMode>::canonicalizations();
    std::vector<const char*> names;
    for (auto& key : canon | std::views::keys) {
        names.push_back(key.c_str());
    }

    const auto render_vulkan = common::settings::get<settings::RenderVulkan>();
    const auto mode = render_vulkan.vSyncMode;
    static int item_current = static_cast<int>(mode);
    ImGui::Combo("vsync mode", &item_current, names.data(), names.size());
    const auto vSyncMode = settings::enums::ToEnum<settings::enums::VSyncMode>(names[item_current]);
    settings::RenderVulkan::setVsyncMode(vSyncMode);
    ImGui::SameLine();
    HelpMarker(std::format("frame 同步方式{}",
                           settings::enums::CanonicalizeEnum<settings::enums::VSyncMode>(mode))
                   .c_str());
}
}  // namespace
namespace graphics::ui {
auto init_debug_info() -> ImguiDebugInfo {
    ImguiDebugInfo debugInfo;
    debugInfo.speed = 90.0F;
    debugInfo.look_x = 2.0f;
    debugInfo.look_y = 2.0f;
    debugInfo.look_z = 2.0F;
    debugInfo.up_z = 1.f;
    debugInfo.up_y = 0.f;
    debugInfo.up_x = 0.f;
    debugInfo.rotate_x = .0f;
    debugInfo.rotate_y = .0f;
    debugInfo.rotate_z = .1f;
    debugInfo.radians = 45.f;
    debugInfo.z_far = .1f;
    debugInfo.z_near = 10.f;
    debugInfo.center_x = 0;
    debugInfo.center_y = 0;
    debugInfo.center_z = 0;
    return debugInfo;
}
void uniform_ui(ImguiDebugInfo& debugInfo) {
    {
        {
            bool show = true;
            ImGui::ShowDemoWindow(&show);
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
            if (ImGui::TreeNode("look at")) {
                ImGui::SliderFloat("x", &debugInfo.look_x, .0f, 8.f);
                ImGui::SliderFloat("y", &debugInfo.look_y, .0f, 8.f);
                ImGui::SliderFloat("z", &debugInfo.look_z, .0f, 8.f);
                ImGui::TreePop();
            }
            if (ImGui::TreeNode("center")) {
                ImGui::SliderFloat("x", &debugInfo.center_x, .0f, center_x);
                ImGui::SliderFloat("y", &debugInfo.center_y, .0f, center_y);
                ImGui::SliderFloat("z", &debugInfo.center_z, .0f, center_z);
                ImGui::TreePop();
            }

            if (ImGui::TreeNode("up")) {
                ImGui::SliderFloat("x", &debugInfo.up_x, .0f, 2.f);
                ImGui::SliderFloat("y", &debugInfo.up_y, .0f, 2.f);
                ImGui::SliderFloat("z", &debugInfo.up_z, .0f, 2.f);
                ImGui::TreePop();
            }

            if (ImGui::TreeNode("rotate")) {
                ImGui::SliderFloat("x", &debugInfo.rotate_x, .0f, 10.f);
                ImGui::SliderFloat("y", &debugInfo.rotate_y, .0f, 10.f);
                ImGui::SliderFloat("z", &debugInfo.rotate_z, .1f, 10.f);
                ImGui::TreePop();
            }
            ImGui::SliderFloat("radians z", &debugInfo.radians, 10.f, 180.f);
            ImGui::SliderFloat("z_near", &debugInfo.z_near, .1f, 10.f);
            ImGui::SliderFloat("z_far", &debugInfo.z_far, .1f, 10.f);

            // 添加恢复默认值的按钮
            if (ImGui::Button("Restore Default")) {
                debugInfo = init_debug_info();
            }
            ImGui::End();
        }
    }
}

auto get_uniform_buffer(ImguiDebugInfo& debugInfo,
                        float extentAspectRation) -> render::UniformBufferObject {
    static auto startTime = ::std::chrono::high_resolution_clock::now();
    auto currentTime = ::std::chrono::high_resolution_clock::now();
    float time =
        ::std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime)
            .count();
    render::UniformBufferObject ubo;
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

void main_ui() {
    static bool show_fps = false;
    ImGui::Begin("main");
    ImGui::Checkbox("show fps", &show_fps);
    ImGui::End();
    if (show_fps) {
        fps();
    }
}

void begin() {}
void end() { ImGui::Render(); }

void pipeline_state(render::PipelineState& state) {
    ImGui::Begin("pipeline 状态");

    if (ImGui::TreeNode("set pipeline state")) {
        ImGui::Checkbox("colorBlendEnable", &state.colorBlendEnable);
        ImGui::Checkbox("logicOpEnable", &state.logicOpEnable);
        ImGui::Checkbox("stencilTestEnable", &state.stencilTestEnable);
        ImGui::Checkbox("depthClampEnable", &state.depthClampEnable);
        ImGui::Checkbox("depthWriteEnable", &state.depthWriteEnable);
        ImGui::Checkbox("depthTestEnable", &state.depthTestEnable);
        ImGui::Checkbox("depthBoundsTestEnable", &state.depthBoundsTestEnable);
        ImGui::Checkbox("cullMode", &state.cullMode);
        ImGui::Checkbox("depthBiasEnable", &state.depthBiasEnable);
        ImGui::Checkbox("rasterizerDiscardEnable", &state.rasterizerDiscardEnable);
        ImGui::Checkbox("primitiveRestartEnable", &state.primitiveRestartEnable);

        ImGui::TreePop();
    }
    if (ImGui::TreeNode("set viewport")) {
        ImGui::SliderFloat("x", &state.viewport.x, .0f, 2048.f);
        ImGui::SliderFloat("y", &state.viewport.y, .0f, 2048.f);
        ImGui::SliderFloat("width", &state.viewport.width, 1.0f, 2048.f);
        ImGui::SliderFloat("height", &state.viewport.height, 1.0f, 2048.f);
        ImGui::SliderFloat("minDepth", &state.viewport.minDepth, .0f, 1.f);
        ImGui::SliderFloat("maxDepth", &state.viewport.maxDepth, .0, 1.f);
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("set scissors")) {
        ImGui::SliderInt("x", &state.scissors.x, 0, 2048);
        ImGui::SliderInt("y", &state.scissors.y, 0, 2048);
        ImGui::SliderInt("width", &state.scissors.width, 1, 2048);
        ImGui::SliderInt("height", &state.scissors.height, 1, 2048);
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("set clear value")) {
        HelpMarker("clear color");
        static float col1[3] = {state.clearColor.r, state.clearColor.g, state.clearColor.b};
        ImGui::ColorEdit3("clear color", col1);
        state.clearColor.r = col1[0];
        state.clearColor.g = col1[1];
        state.clearColor.b = col1[2];
        ImGui::SliderFloat("depth", &state.clearColor.depth, .96f, 1.f);
        ImGui::SliderInt("stencil", &state.clearColor.stencil, 0, 10);
        ImGui::TreePop();
    }

    if (ImGui::TreeNode("set blend color")) {
        ImGui::SliderFloat("r", &state.blendColor.r, .0f, 255.f);
        ImGui::SliderFloat("g", &state.blendColor.g, .0f, 255.f);
        ImGui::SliderFloat("b", &state.blendColor.b, .0f, 1.f);
        ImGui::SliderFloat("a", &state.blendColor.a, .0f, 1.f);
        ImGui::TreePop();
    }

    ImGui::End();
}

void draw_result(ImTextureID imguiTextureID, float aspectRatio) {
    // 设置窗口标志
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                    ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                                    ImGuiWindowFlags_NoBringToFrontOnFocus |
                                    ImGuiWindowFlags_NoNavFocus;

    // 启用停靠空间
    // 获取主窗口的尺寸
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    // ImGui::SetNextWindowPos(viewport->Pos);
    // ImGui::SetNextWindowSize(viewport->Size);
    // ImGui::SetNextWindowViewport(viewport->ID);
    ImGuiIO const& io = ImGui::GetIO();
    (void)io;
    // 在 ImGui 窗口中显示 Vulkan 渲染结果
    ImGui::Begin("Vulkan绘制结果");
    // 获取当前 ImGui 窗口的尺寸
    ImVec2 windowSize = ImGui::GetContentRegionAvail();

    ImVec2 imageSize = calculateAspectRatioSize(windowSize, aspectRatio);
        // 计算图像显示位置，使其居中
        ImVec2 imagePos = ImGui::GetCursorPos();
        imagePos.x += (windowSize.x - imageSize.x) * 0.5f;
        imagePos.y += 0;
        ImGui::SetCursorPos(imagePos);
    ImGui::Image(imguiTextureID, imageSize);
    // 计算文本的宽度
    const char* text = "average %.3f ms/frame (%.1f FPS)";
    ImVec2 textSize = ImGui::CalcTextSize(text);
    // 设置文本绘制位置为右上角
    float textPosX = windowSize.x - textSize.x - 20;  // 10 是右边距
    float textPosY = textSize.y + 10;                 // 20 是上边距
    // // 设置文本绘制位置为右上角
    ImGui::SetCursorPos(ImVec2(textPosX, textPosY));
    ImGui::TextColored({0.0f, 1.0f, 0.0f, 1.0f}, "average %.3f ms/frame (%.1f FPS)",
                       1000.0f / io.Framerate, io.Framerate);
    ImGui::End();
}

void draw_setting() {
    {
        // Using the _simplified_ one-liner Combo() api here
        // See "Combo" section for examples of how to use the more flexible BeginCombo()/EndCombo()
        // api.
        // IMGUI_DEMO_MARKER("Widgets/Basic/Combo");
        ImGui::Begin("系统设置");
        vsync_setting();
        ImGui::End();
    }
}

void draw_docked_window() {
    // 设置窗口标志
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                    ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                                    ImGuiWindowFlags_NoBringToFrontOnFocus |
                                    ImGuiWindowFlags_NoNavFocus;

    // 获取主窗口的尺寸
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::DockSpaceOverViewport();

    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);

    // 创建一个大小跟随主窗口的窗口
    ImGui::Begin("Docked Window", nullptr, window_flags);

    ImGui::End();
}

}  // namespace graphics::ui