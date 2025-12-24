#include "imgui.h"
#include "imGuIZMOquat.h"
#include "gui.hpp"
#include "ecs/components/tag_component.hpp"
#include "ecs/components/render_state_component.hpp"
#include "ecs/components/camera_component.hpp"
#include "ecs/components/dynamic_pipeline_state_component.hpp"
#include "ecs/component.hpp"
#include "ecs/ui/transformUI.hpp"
#include "ecs/ui/cameraUI.hpp"
#include "ecs/ui/lightUI.hpp"

#include <glm/gtc/quaternion.hpp>

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
constexpr float OUTLINER_WIDTH = 400.f;
constexpr float RENDER_STATUS_BAR_HEIGHT = 45.f;

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

void pipeline_state(render::DynamicPipelineState& state) {
    bool color_blend_able = state.colorBlendEnable;
    ImGui::Checkbox("colorBlendEnable", &color_blend_able);
    state.colorBlendEnable = color_blend_able;

    bool stencilTestEnable = state.stencilTestEnable;
    ImGui::Checkbox("stencilTestEnable", &stencilTestEnable);
    state.stencilTestEnable = stencilTestEnable;

    bool depthClampEnable = state.depthClampEnable;
    ImGui::Checkbox("depthClampEnable", &depthClampEnable);
    state.depthClampEnable = depthClampEnable;

    bool depthWriteEnable = state.depthWriteEnable;
    ImGui::Checkbox("depthWriteEnable", &depthWriteEnable);
    state.depthWriteEnable = depthWriteEnable;

    bool depthTestEnable = state.depthTestEnable;
    ImGui::Checkbox("depthTestEnable", &depthTestEnable);
    state.depthTestEnable = depthTestEnable;

    bool depthBoundsTestEnable = state.depthBoundsTestEnable;
    ImGui::Checkbox("depthBoundsTestEnable", &depthBoundsTestEnable);
    state.depthBoundsTestEnable = depthBoundsTestEnable;

    bool cullMode = state.cullMode;
    ImGui::Checkbox("cullMode", &cullMode);
    state.cullMode = cullMode;

    bool depthBiasEnable = state.depthBiasEnable;
    ImGui::Checkbox("depthBiasEnable", &depthBiasEnable);
    state.depthBiasEnable = depthBiasEnable;

    bool rasterizerDiscardEnable = state.rasterizerDiscardEnable;
    ImGui::Checkbox("rasterizerDiscardEnable", &rasterizerDiscardEnable);
    state.rasterizerDiscardEnable = rasterizerDiscardEnable;

    bool primitiveRestartEnable = state.primitiveRestartEnable;
    ImGui::Checkbox("primitiveRestartEnable", &primitiveRestartEnable);
    state.primitiveRestartEnable = primitiveRestartEnable;

    ImGui::NewLine();
    ImGui::Separator();
    ImGui::NewLine();
    ImGui::Text("viewport");
    ImGui::Text("Position & Size");
    ImGui::DragFloat2("Pos (x,y)", &state.viewport.x, 1.0f, 0.0f, 2048.0f);
    ImGui::DragFloat2("Size (w,h)", &state.viewport.width, 1.0f, 1.0f, 12048.0f);
    ImGui::Separator();
    ImGui::Text("Depth Range");
    ImGui::SliderFloat2("Depth (min,max)", &state.viewport.minDepth, 0.0f, 1.0f);

    ImGui::NewLine();
    ImGui::Separator();
    ImGui::NewLine();
    ImGui::Text("scissors");
    ImGui::DragInt2("Pos(x,y)", &state.scissors.x, 1, 0, std::numeric_limits<int>::max());
    ImGui::DragInt2("Size(w,h)", &state.scissors.width, 1, 1, std::numeric_limits<int>::max());
    ImGui::NewLine();
    ImGui::Separator();
    ImGui::NewLine();
    ImGui::Text("blend color");
    ImGui::ColorEdit4("color", &state.blendColor.r);  // NOLINT
}
}  // namespace
namespace graphics::ui {

void draw_texture(settings::MenuData& data, ImTextureID imguiTextureID, float aspectRatio) {
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
        window_size.x  -= OUTLINER_WIDTH;
    }
    if (data.show_status) {
        window_size.y -= RENDER_STATUS_BAR_HEIGHT;
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

void draw_detail(settings::MenuData& data, ecs::Entity entity) {
    ImGui::Begin("Detail", &data.show_detail);
    if (entity.hasComponent<ecs::TagComponent>()) {
        auto& tag = entity.getComponent<ecs::TagComponent>();
        ImGui::TextUnformatted(tag.tag.c_str());
    }
    if (entity.hasComponent<ecs::TransformComponent>()) {
        ImGui::Separator();
        ImGui::Text("TransformComponent");
        auto& tc = entity.getComponent<ecs::TransformComponent>();
        ecs::DrawTransformUI(tc);
        auto qRot = glm::quat(tc.rotation);
        static quat qRot1 = quat(qRot.w, qRot.x, qRot.y, qRot.z);
        qRot1.w = qRot.w;
        qRot1.x = qRot.x;
        qRot1.y = qRot.y;
        qRot1.z = qRot.z;
        ImGui::gizmo3D("##gizmo1", qRot1, 200.f);
        tc.rotation = glm::eulerAngles(glm::quat{qRot1.w, qRot1.x, qRot1.y, qRot1.z});
    }

    if (entity.hasComponent<ecs::CameraComponent>()) {
        ImGui::Separator();
        auto& cam = entity.getComponent<ecs::CameraComponent>();
        ecs::DrawCameraUI(cam);
    }

    if (entity.hasComponent<ecs::DynamicPipeStateComponenet>()) {
        ImGui::Separator();
        ImGui::Text("DynamicPipeState");
        auto& state_component = entity.getComponent<ecs::DynamicPipeStateComponenet>();
        pipeline_state(state_component.state);
    }

    if (entity.hasComponent<ecs::LightComponent>()) {
        ImGui::Separator();
        auto& light = entity.getComponent<ecs::LightComponent>();
        ecs::DrawLightUi(light);
    }

    ImGui::End();
}

// ======================================
// 递归绘制树节点（专业级 Outliner 风格）
// ======================================
void DrawModelTreeNode(settings::MenuData& data, ecs::Entity entity) {
    ImGui::PushID(&entity);
    auto& render_state = entity.getComponent<ecs::RenderStateComponent>();
    auto& tag = entity.getComponent<ecs::TagComponent>();
    const float indent_spacing = ImGui::GetStyle().IndentSpacing;

    // ===== 设置样式 =====
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 2));
    ImGui::BeginGroup();  // 整行布局

    // 缩进
    float cursor_x = ImGui::GetCursorPosX() + indent_spacing;
    ImGui::SetCursorPosX(cursor_x);

    ImGui::Dummy(ImVec2(indent_spacing, 1));
    ImGui::SameLine();

    // ===== 可见性复选框 =====
    ImGui::PushItemFlag(ImGuiItemFlags_NoNav | ImGuiItemFlags_NoTabStop, true);
    ImGui::Checkbox(("##vis" + tag.tag).c_str(), &render_state.visible);
    ImGui::PopItemFlag();
    ImGui::SameLine();

    // ===== 模型名称（可选择区域）=====
    ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0, 0));

    // 使用 Selectable 占满整行，允许重叠
    ImGui::Selectable(tag.tag.c_str(), render_state.is_select(),
                      ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap);
    static unsigned int select_id = std::numeric_limits<unsigned int>::max();
    if (render_state.is_select() && render_state.mouse_select) {
        select_id = render_state.select_id;
    }
    // 仅当点击了名称区域（非箭头/复选框）时才选中
    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
        select_id = render_state.id;
    }
    render_state.select_id = select_id;

    if (render_state.is_select()) {
        if (data.show_detail) {
            draw_detail(data, entity);
        }
    }

    ImGui::PopStyleVar();  // ItemInnerSpacing

    ImGui::EndGroup();
    ImGui::PopStyleVar();  // ItemSpacing

    ImGui::PopID();
}

void showOutliner(world::World& world, settings::MenuData& data) {
    if (data.show_out_liner) {
        // 设置窗口标志
        ImGuiWindowFlags window_flags =
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        auto window_size = viewport->Size;
        window_size.y -= ImGui::GetFrameHeight();
        if (data.show_status) {
            window_size.y -= RENDER_STATUS_BAR_HEIGHT;
        }
        window_size.x = OUTLINER_WIDTH;

        ImVec2 panelPos(viewport->WorkPos.x + (viewport->Size.x - window_size.x),
                        viewport->WorkPos.y);
        ImGui::SetNextWindowSize(window_size);
        ImGui::SetNextWindowPos(panelPos);
        ImGui::Begin("Outliner", &data.show_out_liner, window_flags);
        static int push_id= 0;
        world.processOutlineres([&](ecs::Outliner&& outliner) {
            ecs::Outliner local = std::move(outliner);
            auto& tag = local.entity.getComponent<ecs::TagComponent>();
            ImGuiTreeNodeFlags flags =
                ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
            ImGui::PushID(push_id++); // 使用 entity 的唯一 id
            bool open = ImGui::TreeNodeEx(&local.entity, flags, "%s", tag.tag.c_str());
            // 只有展开时才绘制子节点
            if (open) {
                DrawModelTreeNode(data, local.entity);

                if (!local.children.empty()) {
                    // children with indent
                    ImGui::Indent();
                    for (auto& childEntity : local.children) {
                        DrawModelTreeNode(data, childEntity);
                    }
                    ImGui::Unindent();
                }
                ImGui::TreePop();
            }
            ImGui::PopID();
        });

        ImGui::End();
        push_id = 0;
    }
}

void show_menu(settings::MenuData& data) {
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
            ImGui::MenuItem("\uead8 Imgui Metrics", "", &show_imgui_debug_window);
            ImGui::MenuItem("\uf6c5 Detail", "", &data.show_detail);
            ImGui::MenuItem("\uebf5 状态", "", &data.show_status);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}
auto IsMouseControlledByImGui() -> bool {
    ImGuiIO& io = ImGui::GetIO();
    return io.WantCaptureMouse;
}

void render_status_bar(settings::MenuData& menuData, StatusBarData& barData) {
    if (!menuData.show_status) {
        return;
    }
    auto& io = ImGui::GetIO();
    auto* vp = ImGui::GetMainViewport();

    ImGui::SetNextWindowPos(ImVec2(0, vp->Size.y - RENDER_STATUS_BAR_HEIGHT));
    ImGui::SetNextWindowSize(ImVec2(vp->Size.x, RENDER_STATUS_BAR_HEIGHT));

    ImGui::Begin(
        "##status", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);

    ImGui::TextColored({0.0F, 1.0F, 0.0F, 1.0F}, "FPS: %.1f ", io.Framerate);
    ImGui::SameLine();
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
    ImGui::SameLine();
    ImGui::Text("Mouse: (%.1f, %.1f)", barData.mouseX_, barData.mouseY_);

    ImGui::SameLine();
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
    ImGui::SameLine();
    ImGui::Text("Entities: %d", barData.registry_count);

    ImGui::SameLine();
    ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
    ImGui::SameLine();
    ImGui::Text("device: %s", barData.device_name.c_str());

    if (barData.build_shaders > 0) {
        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();
        ImGui::Text("build shaders: %d", barData.build_shaders);
    }

    ImGui::End();
}
}  // namespace graphics::ui
