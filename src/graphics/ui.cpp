#include "imgui.h"
#include "ui.hpp"

#include "common/settings.hpp"
#include <ranges>
#include <format>

namespace {
constexpr auto MAIN_WINDOW_NAME = "Main Window";

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

void show_fps() {
    static bool show_fps = false;
    ImGui::Checkbox("show fps", &show_fps);
    if (show_fps) {
        fps();
    }
}

auto calculateAspectRatioSize(ImVec2 availableSize, float aspectRatio) -> ImVec2 {
    float targetWidth = availableSize.x;
    float targetHeight = availableSize.x / aspectRatio;

    // 如果高度超出可用区域，则按高度缩放
    if (targetHeight > availableSize.y) {
        targetHeight = availableSize.y;
        targetWidth = availableSize.y * aspectRatio;
    }

    return ImVec2(targetWidth, targetHeight);
}

void HelpMarker(const char* desc) {
    ImGui::TextDisabled("(?)");
    if (ImGui::BeginItemTooltip()) {
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0F);
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
        ImGui::SliderFloat("width", &state.viewport.width, 1.0f, 12048.f);
        ImGui::SliderFloat("height", &state.viewport.height, 1.0f, 12048.f);
        ImGui::SliderFloat("minDepth", &state.viewport.minDepth, .0f, 1.f);
        ImGui::SliderFloat("maxDepth", &state.viewport.maxDepth, .0, 1.f);
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("set scissors")) {
        ImGui::SliderInt("x", &state.scissors.x, 0, 2048);
        ImGui::SliderInt("y", &state.scissors.y, 0, 2048);
        ImGui::SliderInt("width", &state.scissors.width, 1, 12048);
        ImGui::SliderInt("height", &state.scissors.height, 1, 12048);
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
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGuiIO const& io = ImGui::GetIO();
    (void)io;
    // 在 ImGui 窗口中显示 Vulkan 渲染结果
    static bool first_time = true;
    if (first_time) {
        ImGui::SetNextWindowDockID(0x00000002, ImGuiCond_Always);
        first_time = false;
    }
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("绘制结果");
    // 获取当前 ImGui 窗口的尺寸
    ImVec2 windowSize = ImGui::GetContentRegionAvail();

    ImVec2 imageSize = calculateAspectRatioSize(windowSize, aspectRatio);
    // 计算图像显示位置，使其居中
    ImVec2 imagePos = ImGui::GetCursorPos();
    ImGui::SetCursorPos(imagePos);
    ImGui::Image(imguiTextureID, windowSize);
    // 计算文本的宽度
    const auto* text = "average %.3f ms/frame (%.1f FPS)";
    const ImVec2 textSize = ImGui::CalcTextSize(text);
    // 设置文本绘制位置为右上角
    const float textPosX = windowSize.x - textSize.x - 20;  // 10 是右边距
    const float textPosY = textSize.y + 10;                 // 20 是上边距
    // // 设置文本绘制位置为右上角
    ImGui::SetCursorPos(ImVec2(textPosX, textPosY));
    ImGui::TextColored({0.0f, 1.0f, 0.0f, 1.0f}, text, 1000.0f / io.Framerate, io.Framerate);
    ImGui::PopStyleVar();
    ImGui::End();
}

void draw_setting() {
    {
        // Using the _simplified_ one-liner Combo() api here
        // See "Combo" section for examples of how to use the more flexible BeginCombo()/EndCombo()
        // api.
        // IMGUI_DEMO_MARKER("Widgets/Basic/Combo");
        ImGui::Begin("系统设置");
        show_fps();
        vsync_setting();
        ImGui::End();
    }
}

}  // namespace graphics::ui
