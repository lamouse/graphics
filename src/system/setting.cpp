module;
#include "common/settings.hpp"
#include "ui/ui.hpp"
#include <imgui.h>
#include <format>
#include <ranges>
#include <vector>

module setting;
namespace  {

void vsync_setting() {
    auto canon = settings::enums::EnumMetadata<settings::enums::VSyncMode>::canonicalizations();
    std::vector<const char*> names;
    for (auto& key : canon | std::views::keys) {
        names.push_back(key.c_str());
    }

    const auto render_vulkan = common::settings::get<settings::RenderVulkan>();
    const auto mode = render_vulkan.vSyncMode;
    static int item_current = static_cast<int>(mode);
    ImGui::Combo("vsync mode", &item_current, names.data(), static_cast<int>(names.size()));
    const auto vSyncMode = settings::enums::ToEnum<settings::enums::VSyncMode>(names[item_current]);
    settings::RenderVulkan::setVsyncMode(vSyncMode);
    ImGui::SameLine();
    HelpMarker(std::format("frame 同步方式{}",
                           settings::enums::CanonicalizeEnum<settings::enums::VSyncMode>(mode))
                   .c_str());
}

void fps() {
    ImGuiIO const& io = ImGui::GetIO();
    (void)io;
    ImVec2 main_pos = ImGui::GetMainViewport()->Pos;
    auto main_size = ImGui::GetMainViewport()->Size;
    bool fps_open = false;
    ImGui::SetNextWindowBgAlpha(.0f);
    ImGui::SetNextWindowPos({main_pos.x + main_size.x, main_pos.y}, ImGuiCond_Always,
                            ImVec2(1.01F, 0.0F));
    ImGui::Begin("Window 1", &fps_open,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoMouseInputs | ImGuiWindowFlags_NoScrollbar |
                     ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoBackground |
                     ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::TextColored({0.0F, 1.0F, 0.0F, 1.0F}, "FPS: %.1f ", io.Framerate);
    ImGui::End();
}
void show_fps() {
    static bool show_fps = false;
    ImGui::Checkbox("show fps", &show_fps);
    if (show_fps) {
        fps();
    }
}
}
namespace graphics {
void draw_setting() {
    ImGui::Begin("系统设置");
    show_fps();
    vsync_setting();
    ImGui::End();
}
}  // namespace graphics