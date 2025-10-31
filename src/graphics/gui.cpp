#include "imgui.h"
#include "gui.hpp"
#include "ecs/components/tag_component.hpp"
#include "ecs/components/render_state_component.hpp"
#include <ImGuizmo.h>
#include <GraphEditor.h>
#include <ImSequencer.h>
#include <ImCurveEdit.h>
#include <ImZoomSlider.h>
#include <limits>
// Helper to wire demo markers located in code to an interactive browser
typedef void (*ImGuiDemoMarkerCallback)(const char* file, int line, const char* section,
                                        void* user_data);
extern ImGuiDemoMarkerCallback GImGuiDemoMarkerCallback;
extern void* GImGuiDemoMarkerCallbackUserData;
ImGuiDemoMarkerCallback GImGuiDemoMarkerCallback = NULL;
void* GImGuiDemoMarkerCallbackUserData = NULL;
#define IMGUI_DEMO_MARKER(section)                                      \
    do {                                                                \
        if (GImGuiDemoMarkerCallback != NULL)                           \
            GImGuiDemoMarkerCallback(__FILE__, __LINE__, section,       \
                                     GImGuiDemoMarkerCallbackUserData); \
    } while (0)
namespace {
constexpr float OUTLINER_WIDTH = 0.3f;
// ======================================
// 手动绘制箭头函数（替代不存在的 ImGui::RenderArrow）
// ======================================
void RenderArrow(ImDrawList* draw_list, ImVec2 pos, ImU32 col, ImGuiDir dir, float size = 5.0f) {
    std::array<ImVec2, 3> points;
    pos = ImVec2(floorf(pos.x) + 0.5F, floorf(pos.y) + 0.5F);

    switch (dir) {
        case ImGuiDir_Left:
            points[0] = ImVec2(pos.x + size, pos.y - size);
            points[1] = ImVec2(pos.x + size, pos.y + size);
            points[2] = ImVec2(pos.x - size, pos.y);
            break;
        case ImGuiDir_Right:
            points[0] = ImVec2(pos.x - size, pos.y - size);
            points[1] = ImVec2(pos.x - size, pos.y + size);
            points[2] = ImVec2(pos.x + size, pos.y);
            break;
        case ImGuiDir_Up:
            points[0] = ImVec2(pos.x - size, pos.y + size);
            points[1] = ImVec2(pos.x + size, pos.y + size);
            points[2] = ImVec2(pos.x, pos.y - size);
            break;
        case ImGuiDir_Down:
            points[0] = ImVec2(pos.x - size, pos.y - size);
            points[1] = ImVec2(pos.x + size, pos.y - size);
            points[2] = ImVec2(pos.x, pos.y + size);
            break;
        default:
            return;
    }

    draw_list->AddTriangleFilled(points[0], points[1], points[2], col);
}

// Note that shortcuts are currently provided for display only
// (future version will add explicit flags to BeginMenu() to request processing shortcuts)
static void ShowExampleMenuFile() {
    IMGUI_DEMO_MARKER("Examples/Menu");
    ImGui::MenuItem("(demo menu)", NULL, false, false);
    if (ImGui::MenuItem("New")) {
    }
    if (ImGui::MenuItem("Open", "Ctrl+O")) {
    }
    if (ImGui::BeginMenu("Open Recent")) {
        ImGui::MenuItem("fish_hat.c");
        ImGui::MenuItem("fish_hat.inl");
        ImGui::MenuItem("fish_hat.h");
        if (ImGui::BeginMenu("More..")) {
            ImGui::MenuItem("Hello");
            ImGui::MenuItem("Sailor");
            if (ImGui::BeginMenu("Recurse..")) {
                ShowExampleMenuFile();
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenu();
    }
    if (ImGui::MenuItem("Save", "Ctrl+S")) {
    }
    if (ImGui::MenuItem("Save As..")) {
    }

    ImGui::Separator();
    IMGUI_DEMO_MARKER("Examples/Menu/Options");
    if (ImGui::BeginMenu("Options")) {
        static bool enabled = true;
        ImGui::MenuItem("Enabled", "", &enabled);
        ImGui::BeginChild("child", ImVec2(0, 60), ImGuiChildFlags_Borders);
        for (int i = 0; i < 10; i++) ImGui::Text("Scrolling Text %d", i);
        ImGui::EndChild();
        static float f = 0.5f;
        static int n = 0;
        ImGui::SliderFloat("Value", &f, 0.0f, 1.0f);
        ImGui::InputFloat("Input", &f, 0.1f);
        ImGui::Combo("Combo", &n, "Yes\0No\0Maybe\0\0");
        ImGui::EndMenu();
    }

    IMGUI_DEMO_MARKER("Examples/Menu/Colors");
    if (ImGui::BeginMenu("Colors")) {
        float sz = ImGui::GetTextLineHeight();
        for (int i = 0; i < ImGuiCol_COUNT; i++) {
            const char* name = ImGui::GetStyleColorName((ImGuiCol)i);
            ImVec2 p = ImGui::GetCursorScreenPos();
            ImGui::GetWindowDrawList()->AddRectFilled(p, ImVec2(p.x + sz, p.y + sz),
                                                      ImGui::GetColorU32((ImGuiCol)i));
            ImGui::Dummy(ImVec2(sz, sz));
            ImGui::SameLine();
            ImGui::MenuItem(name);
        }
        ImGui::EndMenu();
    }

    // Here we demonstrate appending again to the "Options" menu (which we already created above)
    // Of course in this demo it is a little bit silly that this function calls BeginMenu("Options")
    // twice. In a real code-base using it would make senses to use this feature from very different
    // code locations.
    if (ImGui::BeginMenu("Options"))  // <-- Append!
    {
        IMGUI_DEMO_MARKER("Examples/Menu/Append to an existing menu");
        static bool b = true;
        ImGui::Checkbox("SomeOption", &b);
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Disabled", false))  // Disabled
    {
        IM_ASSERT(0);
    }
    if (ImGui::MenuItem("Checked", nullptr, true)) {
    }
    ImGui::Separator();
    if (ImGui::MenuItem("Quit", "Alt+F4")) {
    }
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

void draw_result(MenuData& data, ImTextureID imguiTextureID, float aspectRatio) {
    // 设置窗口标志
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                    ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                                    ImGuiWindowFlags_NoBringToFrontOnFocus |
                                    ImGuiWindowFlags_NoNavFocus;

    // 启用停靠空间
    // 获取主窗口的尺寸
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    auto window_size = viewport->Size;
    if (data.show_out_liner) {
        window_size.x *= (1.f - OUTLINER_WIDTH);
    }
    window_size.y -= ImGui::GetFrameHeight();
    ImGui::SetNextWindowSize(window_size);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("绘制结果", nullptr, window_flags);

    // 计算要裁剪的区域（保持宽高比）
    float win_ratio = window_size.x / window_size.y;

    float uv_min_x = 0.0f, uv_min_y = 0.0f;
    float uv_max_x = 1.0f, uv_max_y = 1.0f;

    if (win_ratio > aspectRatio) {
        // 窗口更宽 → 垂直方向裁剪
        float h = aspectRatio / win_ratio;
        float pad = (1.0f - h) * 0.5f;
        uv_min_y = pad;
        uv_max_y = 1.0f - pad;
    } else {
        // 窗口更高 → 水平方向裁剪
        float w = win_ratio / aspectRatio;
        float pad = (1.0f - w) * 0.5f;
        uv_min_x = pad;
        uv_max_x = 1.0f - pad;
    }

    ImGui::Image(imguiTextureID, window_size, ImVec2(uv_min_x, uv_min_y),
                 ImVec2(uv_max_x, uv_max_y));

    ImGui::PopStyleVar();
    ImGui::End();
}

void init_imgui(float scale) {
    // 这里使用了imgui的一个分支docking
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;      // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;    // Enable Multi-Viewport / Platform
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

// ======================================
// 递归绘制树节点（专业级 Outliner 风格）
// ======================================
void DrawModelTreeNode(ecs::Entity entity, int depth = 0) {
    ImGui::PushID(&entity);
    auto& render_state = entity.getComponent<ecs::RenderStateComponent>();
    auto& tag = entity.getComponent<ecs::TagComponent>();
    // const bool hasChildren = model.hasChildren();
    const float indent_spacing = ImGui::GetStyle().IndentSpacing;
    const float line_height = ImGui::GetFrameHeight();

    // ===== 设置样式 =====
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 2));
    ImGui::BeginGroup();  // 整行布局

    // 缩进
    float cursor_x = ImGui::GetCursorPosX() + depth * indent_spacing;
    ImGui::SetCursorPosX(cursor_x);

    // ===== 折叠箭头（如果有子项）=====
    // if (hasChildren) {
    //     ImVec2 p_min = ImGui::GetCursorScreenPos();
    //     ImVec2 btn_size(indent_spacing, line_height);

    //     // 创建不可见按钮
    //     if (ImGui::InvisibleButton("toggle", btn_size)) {
    //         model.expanded = !model.expanded;
    //     }

    //     // 绘制箭头 ▶ / ▼
    //     ImDrawList* draw_list = ImGui::GetWindowDrawList();
    //     ImU32 text_col = ImGui::GetColorU32(ImGuiCol_Text);
    //     float arrow_size = (line_height - 4.0f) * 0.5f;
    //     ImVec2 center_offset(btn_size.x * 0.5f - arrow_size,
    //                          (line_height - arrow_size * 2.0f) * 0.5f);

    //     ImGuiDir dir = model.expanded ? ImGuiDir_Down : ImGuiDir_Right;
    //     RenderArrow(draw_list,
    //                 ImVec2(p_min.x + center_offset.x, p_min.y + center_offset.y + arrow_size),
    //                 text_col, dir, arrow_size);

    //     ImGui::SameLine();
    // } else {
    // 无子项，占位
    ImGui::Dummy(ImVec2(indent_spacing, 1));
    ImGui::SameLine();
    //}

    // ===== 可见性复选框 =====
    ImGui::PushItemFlag(ImGuiItemFlags_NoNav | ImGuiItemFlags_NoTabStop, true);
    ImGui::Checkbox(("##vis" + tag.tag).c_str(), &render_state.visible);
    ImGui::PopItemFlag();
    ImGui::SameLine();

    // ===== 模型名称（可选择区域）=====
    ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0, 0));

    // 使用 Selectable 占满整行，允许重叠
    ImGui::Selectable(tag.tag.c_str(), false,
                      ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap);
    static unsigned int select_id = std::numeric_limits<unsigned int>::max();
    // 仅当点击了名称区域（非箭头/复选框）时才选中
    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
        select_id = render_state.id;
    }
    render_state.select_id = select_id;

    ImGui::PopStyleVar();  // ItemInnerSpacing

    // // ===== 子项数量（可选）=====
    // if (hasChildren && model.expanded) {
    //     ImGui::SameLine();
    //     ImGui::TextDisabled("(%d)", (int)model.children.size());
    // }

    ImGui::EndGroup();
    ImGui::PopStyleVar();  // ItemSpacing

    // ===== 递归绘制子项 =====
    // if (hasChildren && model.expanded) {
    //     for (auto& child : model.children) {
    //         DrawModelTreeNode(child, depth + 1);
    //     }
    // }

    ImGui::PopID();
}

// ======================================
// 主函数：显示 Outliner 窗口
// ======================================
void ShowOutliner(std::span<ecs::Entity> instances, bool& show) {
    if (!show) {
        return;
    }
    // 设置窗口标志
    ImGuiWindowFlags window_flags =
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    auto window_size = viewport->Size;
    window_size.y -= ImGui::GetFrameHeight();
    window_size.x *= OUTLINER_WIDTH;

    ImVec2 panelPos(viewport->WorkPos.x + (viewport->Size.x - window_size.x), viewport->WorkPos.y);
    ImGui::SetNextWindowSize(window_size);
    ImGui::SetNextWindowPos(panelPos);
    ImGui::Begin("Outliner", &show, window_flags);

    for (auto& instance : instances) {
        DrawModelTreeNode(instance, 0);
    }

    ImGui::End();
}
void show_menu(MenuData& data) {
    static bool show_fps = false;
    if (show_fps) {
        fps();
    }
    static bool show_imgui_debug_window = false;
    if (show_imgui_debug_window) {
        ImGui::ShowMetricsWindow(&show_imgui_debug_window);
    }
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            ShowExampleMenuFile();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Undo", "CTRL+Z")) {
            }
            if (ImGui::MenuItem("Redo", "CTRL+Y", false, false)) {
            }  // Disabled item
            ImGui::Separator();
            if (ImGui::MenuItem("Cut", "CTRL+X")) {
            }
            if (ImGui::MenuItem("Copy", "CTRL+C")) {
            }
            if (ImGui::MenuItem("Paste", "CTRL+V")) {
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Window")) {
            ImGui::MenuItem("\uf1b3 outliner", nullptr, &data.show_out_liner);
            ImGui::MenuItem("\ueb51 系统设置", "", &data.show_system_setting);
            ImGui::MenuItem("\uF15C 日志", "", &data.show_log);
            ImGui::MenuItem("\uf9c4 fps", "", &show_fps);
            ImGui::MenuItem("\uead8 Imgui Metrics", "", &show_imgui_debug_window);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}
}  // namespace graphics::ui
