#include "gui.hpp"
#include "common/file.hpp"
#include "imgui.h"
#include "imGuIZMOquat.h"

#include "ecs/components/tag_component.hpp"
#include "ecs/components/render_state_component.hpp"
#include "ecs/components/camera_component.hpp"
#include "ecs/components/dynamic_pipeline_state_component.hpp"
#include "ecs/component.hpp"
#include "ecs/ui/transformUI.hpp"
#include "ecs/ui/cameraUI.hpp"
#include "ecs/ui/lightUI.hpp"
#include "effects/light/point_light.hpp"
#include "effects/cubemap/skybox.hpp"
#include <string>

#include <glm/gtc/quaternion.hpp>
#include <queue>
#include <mutex>
#include <filesystem>

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
ecs::Entity detail_entity;

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
struct OutLinerMenuData {
        bool add_point_light_window{};
        bool add_sky_box_window{};
        bool has_sky_box{};
        bool add_model{};
};

std::queue<std::function<void()>> imgui_event_funcs;
std::mutex imgui_event_mutex;
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
        window_size.x -= OUTLINER_WIDTH;
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

void draw_detail(settings::MenuData& data, ecs::Entity entity) {
    if (!data.show_detail || !entity) {
        return;
    }
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

void DrawModelTreeNode(ecs::Entity entity) {
    auto& render_state = entity.getComponent<ecs::RenderStateComponent>();
    ImGui::PushID(&render_state);
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
    static uint32_t select = std::numeric_limits<uint32_t>::max();

    // 使用 Selectable 占满整行，允许重叠
    ImGui::Selectable(tag.tag.c_str(), render_state.is_select(),
                      ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap);

    // 检查是否右键点击了该 Selectable，并弹出上下文菜单
    auto popup_tag = std::string("##tag_context_menu") + std::to_string(render_state.id);
    if (ImGui::BeginPopupContextItem(popup_tag.c_str(), ImGuiPopupFlags_MouseButtonRight)) {
        if (ImGui::MenuItem("编辑")) {
            settings::values.menu_data.show_detail = true;
            detail_entity = entity;
            select = render_state.id;
        }
        if (ImGui::MenuItem("删除")) {
            // 处理“删除”操作
        }
        // 可以添加更多菜单项
        ImGui::EndPopup();
    }

    // 仅当点击了名称区域（非箭头/复选框）时才选中
    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
        if (render_state.is_select()) {
            select = std::numeric_limits<uint32_t>::max();
            detail_entity = {};
        } else {
            select = render_state.id;
        }
    }
    render_state.select_id = select;

    ImGui::PopStyleVar();  // ItemInnerSpacing

    ImGui::EndGroup();
    ImGui::PopStyleVar();  // ItemSpacing

    ImGui::PopID();
}

void DrawOutlinerTree(const ecs::Outliner& outliner, int id) {
    auto& tag = outliner.entity.getComponent<ecs::TagComponent>();
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
    ImGui::PushID(id);  // 使用 entity 的唯一 id
    static uint32_t select_id = std::numeric_limits<unsigned int>::max();
    auto find_detail_child = std::ranges::find(outliner.children.begin(), outliner.children.end(),
                                               detail_entity) != outliner.children.end();

    bool open{};
    // 判断点击事件
    if (outliner.entity.hasComponent<ecs::RenderStateComponent>()) {
        auto& render_state = outliner.entity.getComponent<ecs::RenderStateComponent>();
        if (find_detail_child) {
            select_id = render_state.id;
        }
        if (render_state.mouse_select) {
            select_id = render_state.id;
            render_state.select_id = select_id;
        }
        if (render_state.is_select()) {
            flags |= ImGuiTreeNodeFlags_Selected;
        }

        ImGui::PushItemFlag(ImGuiItemFlags_NoNav | ImGuiItemFlags_NoTabStop, true);
        ImGui::Checkbox(("##vis" + tag.tag).c_str(), &render_state.visible);
        ImGui::PopItemFlag();
        ImGui::SameLine();
        open = ImGui::TreeNodeEx(&outliner.entity, flags, "%s", tag.tag.c_str());
        auto popup_tag = std::string("##tag_context_menu") + std::to_string(render_state.id);
        if (ImGui::BeginPopupContextItem(popup_tag.c_str(), ImGuiPopupFlags_MouseButtonRight)) {
            if (ImGui::MenuItem("编辑")) {
                detail_entity = outliner.entity;
                select_id = render_state.id;
            }
            if (ImGui::MenuItem("删除")) {
                // 处理“删除”操作
            }
            // 可以添加更多菜单项
            ImGui::EndPopup();
        }
        if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && !ImGui::IsItemToggledOpen()) {
            if (render_state.is_select()) {
                select_id = std::numeric_limits<unsigned int>::max();
                detail_entity = {};
            } else {
                select_id = render_state.id;
                render_state.mouse_select = false;
            }
        }
        render_state.select_id = select_id;
    } else {
        open = ImGui::TreeNodeEx(&outliner.entity, flags, "%s", tag.tag.c_str());
    }

    if (open) {
        if (!outliner.children.empty()) {
            // children with indent
            ImGui::Indent();
            for (const auto& childEntity : outliner.children) {
                DrawModelTreeNode(childEntity);
            }
            ImGui::Unindent();
        }
        ImGui::TreePop();
    }
    ImGui::PopID();
}

void add_point_light(world::World& world, ResourceManager& resourceManager, bool* open) {
    ImGui::Begin("点光源", open);

    static glm::vec3 color{1.f};
    static float intensity{10.f};
    static float radius{.04f};
    ImGui::ColorEdit3("颜色", glm::value_ptr(color));
    ImGui::SliderFloat("强度", &intensity, 10, 100);
    ImGui::SliderFloat("半径", &radius, 0.02f, 0.40f);
    if (ImGui::Button("确认")) {
        auto point_light = std::make_shared<graphics::effects::PointLightEffect>(
            resourceManager, intensity, radius, color);
        world.addDrawable(point_light);
        color = glm::vec3{1.f};
        intensity = 10.f;
        radius = .04f;
        (*open) = false;
    }

    ImGui::SameLine();
    if (ImGui::Button("取消")) {
        *open = false;
    }

    ImGui::End();
}

auto add_sky_box_window(world::World& world, ResourceManager& resourceManager, bool* open) -> bool {
    ImGui::Begin("sky box", open);

    if (ImGui::Button("确认")) {
        auto sky_box = std::make_shared<graphics::effects::SkyBox>(resourceManager);
        world.addDrawable(sky_box);
        (*open) = false;
        ImGui::End();
        return true;
    }

    ImGui::SameLine();
    if (ImGui::Button("取消")) {
        *open = false;
    }

    ImGui::End();
    return false;
}

void showOutliner(world::World& world, ResourceManager& resourceManager, settings::MenuData& data) {
    static OutLinerMenuData outLinerMenuData;
    imguiGizmo::setGizmoFeelingRot(
        .75f);  // default 1.0, >1 more mouse sensitivity, <1 less mouse sensitivity
    imguiGizmo::setPanScale(.5f);    // default 1.0, >1 more, <1 less
    imguiGizmo::setDollyScale(.5f);  // default 1.0, >1 more, <1 less
    imguiGizmo::setDollyWheelScale(
        .5f);  // default 1.0, > more, < less ... (from v3.1 separate values)
    imguiGizmo::setPanModifier(
        vg::evSuperModifier);  // change KEY modifier: CTRL (default) ==> SUPER
    imguiGizmo::setDollyModifier(
        vg::evControlModifier);  // change KEY modifier: SHIFT (default) ==> CTRL

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
        static int push_id = 0;
        world.processOutlineres([&](ecs::Outliner&& outliner) {
            ecs::Outliner local = std::move(outliner);
            DrawOutlinerTree(local, push_id++);
        });
        draw_detail(settings::values.menu_data, detail_entity);
        if (ImGui::BeginPopupContextWindow(
                "Outliner", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems)) {
            if (ImGui::BeginMenu("添加")) {
                if (ImGui::BeginMenu("光源")) {
                    ImGui::MenuItem("点光源", nullptr, &outLinerMenuData.add_point_light_window);
                    ImGui::EndMenu();
                }
                ImGui::BeginDisabled(outLinerMenuData.has_sky_box);
                ImGui::MenuItem("天空盒子", nullptr, &outLinerMenuData.add_sky_box_window);
                ImGui::MenuItem("模型", nullptr, &outLinerMenuData.add_model);
                ImGui::EndDisabled();
                ImGui::EndMenu();
            }
            if (ImGui::MenuItem("Save")) { /* ... */
            }
            ImGui::EndPopup();
        }
        ImGui::End();
        push_id = 0;
    }

    if (outLinerMenuData.add_point_light_window) {
        add_point_light(world, resourceManager, &outLinerMenuData.add_point_light_window);
    }
    if (outLinerMenuData.add_sky_box_window) {
        outLinerMenuData.has_sky_box =
            add_sky_box_window(world, resourceManager, &outLinerMenuData.add_sky_box_window);
    }
    if (outLinerMenuData.add_model) {
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
    if (ImGui::GetCurrentContext() == nullptr) {
        return false;
    }
    ImGuiIO& io = ImGui::GetIO();
    return io.WantCaptureMouse;
}

auto IsKeyboardControlledByImGui() -> bool {
    if (ImGui::GetCurrentContext() == nullptr) {
        return false;
    }
    ImGuiIO& io = ImGui::GetIO();
    return io.WantCaptureKeyboard;
}

void add_imgui_event(const std::function<void()>& event_func) {
    if (!settings::values.use_debug_ui) {
        return;
    }
    if (ImGui::GetCurrentContext() == nullptr) {
        return;
    }
    std::scoped_lock<std::mutex> lock(imgui_event_mutex);
    imgui_event_funcs.push(event_func);
}
void run_all_imgui_event() {
    if (!settings::values.use_debug_ui) {
        return;
    }
    std::scoped_lock<std::mutex> lock(imgui_event_mutex);
    if (imgui_event_funcs.empty()) {
        return;
    }
    while (!imgui_event_funcs.empty()) {
        auto func = imgui_event_funcs.front();
        func();
        imgui_event_funcs.pop();
    }
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
    auto bar_color_text = [](ImVec4 color, const char* fmt, ...) -> void {  // NOLINT
        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();
        va_list args;
        va_start(args, fmt);
        ImGui::TextColoredV(color, fmt, args);
        va_end(args);
    };
    auto bar_text = [](const char* fmt, ...) -> void {  // NOLINT
        ImGui::SameLine();
        ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
        ImGui::SameLine();
        va_list args;
        va_start(args, fmt);
        ImGui::TextV(fmt, args);
        va_end(args);
    };

    bar_color_text({0.0F, 1.0F, 0.0F, 1.0F}, "FPS: %.1f ", io.Framerate);
    bar_text("Mouse: (%.1f, %.1f)", barData.mouseX_, barData.mouseY_);
    bar_text("Entities: %d", barData.registry_count);
    bar_text("device: %s", barData.device_name.c_str());
    bar_text("scaling filter: %s",
             settings::enums::CanonicalizeEnum(settings::values.scaling_filter.GetValue()).c_str());

    if (barData.build_shaders > 0) {
        bar_color_text({0.3f, .1f, 1.f, .0f}, "build shaders: %d", barData.build_shaders);
    }

    ImGui::End();
}

auto add_model(std::string_view model_path) -> AddModelInfo {
    static bool confirm_add_model = true;
    confirm_add_model = true;
    ASSERT_MSG(!model_path.empty(), "model path is empty");  // NOLINT
    ImGui::Begin("Add Model", &confirm_add_model);
    ImGui::Text("Model Path: %s", model_path.data());
    auto mtl_path = common::model_mtl_file_path(std::string(model_path));
    if (not mtl_path.empty()) {
        ImGui::Text("MTL Path: %s", mtl_path.c_str());
    }
    ImGui::Separator();
    static std::unique_ptr<effects::ModelEffectInfo> model_info;
    if(not model_info){
        model_info = std::make_unique<effects::ModelEffectInfo>();
    }
    auto model_path_fs = std::filesystem::path(model_path);
    model_info->source_path = std::string(model_path);
    model_info->model_name = model_path_fs.filename().string();
    model_info->asset_name = model_path_fs.stem().string() + ".asset";
    model_info->shader_name = "model";
    ImGui::Text("asset name: %s", model_info->asset_name.c_str());
    ImGui::Checkbox("flip uv", &model_info->flip_uv);
    ImGui::Checkbox("use mtl", &model_info->use_mtl);
    ImGui::Checkbox("copy local", &model_info->copy_local);
    ImGui::Checkbox("split mesh", &model_info->split_mesh);
    ImGui::SliderFloat("scale", &model_info->default_scale, 0.01f, 10.f);
    static std::unique_ptr<render::DynamicPipelineState> current_pipeline_state;
    if (!current_pipeline_state) {
        current_pipeline_state = std::make_unique<render::DynamicPipelineState>(model_info->pipeline_state);
    }
    ImGui::Separator();
    ImGui::Text("Pipeline State:");
    pipeline_state(*current_pipeline_state);
    bool use_model{};
    if (ImGui::Button("确认")) {
        std::filesystem::path model(model_path);
        use_model = true;
        if(model_info->copy_local){
            model_info->source_path = (common::get_module_path(common::ModuleType::Model) / model_path_fs.filename()).string();
        };
        common::copy_file(model,
                          common::get_module_path(common::ModuleType::Model) / model.filename());

        if (!mtl_path.empty()) {
            common::copy_file(mtl_path, common::get_module_path(common::ModuleType::Model) /
                                            std::filesystem::path(mtl_path).filename());
        }
        model_info->pipeline_state = *current_pipeline_state;
        current_pipeline_state.reset();
        confirm_add_model = false;
    }
    ImGui::SameLine();
    if (ImGui::Button("取消")) {
        spdlog::info("cancel add model {}", model_path);
        confirm_add_model = false;
    }
    ImGui::End();
    if (use_model) {
        auto tmp = std::move(*model_info);
        model_info.reset();
        return tmp;
    }
    if(!confirm_add_model){
        current_pipeline_state.reset();
        model_info.reset();
    }
    return !confirm_add_model;
}

}  // namespace graphics::ui
