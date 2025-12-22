#include <utility>
#include "common/common_funcs.hpp"
#include "graphics/QT_render_widget.hpp"
#include "graphics/QT_window.hpp"
#include "graphics/QT_common.hpp"
#include "input/mouse.h"
#include "input/input.hpp"
#include <QWindow>
#include <Qlayout>
#include <QMouseEvent>
#include <imgui.h>
namespace graphics {

namespace {
class RenderWidget : public QWidget {
    public:
        CLASS_NON_MOVEABLE(RenderWidget);
        CLASS_NON_COPYABLE(RenderWidget);
        explicit RenderWidget(RenderWindow* parent) : QWidget(parent) {
            setAttribute(Qt::WA_NativeWindow);
            setAttribute(Qt::WA_PaintOnScreen);
            if (qt::get_window_system_type() == core::frontend::WindowSystemType::Wayland) {
                setAttribute(Qt::WA_DontCreateNativeAncestors);
            }
        }

        virtual ~RenderWidget() = default;

        [[nodiscard]] auto paintEngine() const -> QPaintEngine* override { return nullptr; }
};

struct VulkanRenderWidget : public RenderWidget {
        CLASS_NON_MOVEABLE(VulkanRenderWidget);
        CLASS_NON_COPYABLE(VulkanRenderWidget);
        explicit VulkanRenderWidget(RenderWindow* parent) : RenderWidget(parent) {
            windowHandle()->setSurfaceType(QWindow::VulkanSurface);
        }
        ~VulkanRenderWidget() override = default;
};

}  // namespace

RenderWindow::RenderWindow(QTWindow* parent, std::shared_ptr<input::InputSystem> input_system)
    : QWidget(parent), input_system_(std::move(input_system)) {
    setAttribute(Qt::WA_AcceptTouchEvents);
    auto* layout = new QHBoxLayout(this);  // NOLINT
    layout->setContentsMargins(0, 0, 0, 0);
    this->setMouseTracking(true);
    connect(this, &RenderWindow::FirstFrameDisplayed, parent, &QTWindow::OnLoadComplete);
    connect(this, &RenderWindow::ExitSignal, parent, &QTWindow::OnExit, Qt::QueuedConnection);
    connect(this, &RenderWindow::ExecuteProgramSignal, parent, &QTWindow::OnExecuteProgram,
            Qt::QueuedConnection);
}
auto RenderWindow::IsLoadingComplete() const -> bool { return first_frame; }
auto RenderWindow::IsShown() const -> bool { return this->isVisible(); }
auto RenderWindow::IsMinimized() const -> bool { return this->isMinimized(); }
auto RenderWindow::shouldClose() const -> bool { return should_close_; }
void RenderWindow::OnFrameDisplayed() {
    if (!first_frame) {
        first_frame = true;
        emit FirstFrameDisplayed();
    }
}

void RenderWindow::closeEvent(QCloseEvent* event) {
    should_close_ = true;
    emit CLosed();
    QWidget::closeEvent(event);
}
void RenderWindow::leaveEvent(QEvent* event) { QWidget::leaveEvent(event); }

void RenderWindow::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    OnFramebufferResized();
}
void RenderWindow::Exit() { emit ExitSignal(); }
void RenderWindow::ExecuteProgram(std::size_t program_index) {
    emit ExecuteProgramSignal(program_index);
}
auto RenderWindow::windowPixelRatio() const -> qreal { return devicePixelRatioF(); }

auto RenderWindow::InitRenderTarget() -> bool {
    ReleaseRenderTarget();
    first_frame = false;
    if (!initVulkanRenderWidget()) {
        return false;
    }
    int width = 1920, height = 1080;
    window_info = qt::get_window_system_info(child_widget_->windowHandle());
    child_widget_->resize(width, height);
    layout()->addWidget(child_widget_);
    setMinimumSize(1, 1);
    resize(width, height);

    OnFramebufferResized();
    BackupGeometry();
    return true;
}
void RenderWindow::ReleaseRenderTarget() {
    if (child_widget_) {
        layout()->removeWidget(child_widget_);
        child_widget_->deleteLater();
        child_widget_ = nullptr;
    }
}
void RenderWindow::BackupGeometry() { geometry_ = QWidget::saveGeometry(); }

auto RenderWindow::initVulkanRenderWidget() -> bool {
    child_widget_ = new VulkanRenderWidget(this);  // NOLINT
    child_widget_->windowHandle()->create();
    if (!child_widget_) {
        return false;
    }
    return true;
}

void RenderWindow::OnFramebufferResized() {
    const qreal pixel_ratio = child_widget_->devicePixelRatioF();
    const auto width = static_cast<std::uint32_t>(child_widget_->width() * pixel_ratio);
    const auto height = static_cast<std::uint32_t>(child_widget_->height() * pixel_ratio);
    QSize logicalSize = this->size();
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize =
        ImVec2(static_cast<float>(logicalSize.width()), static_cast<float>(logicalSize.height()));
    UpdateCurrentFramebufferLayout(width, height);
}

void RenderWindow::mousePressEvent(QMouseEvent* event) {
    // Touch input is handled in TouchBeginEvent
    if (event->source() == Qt::MouseEventSynthesizedBySystem) {
        return;
    }
    const auto pos = event->pos();
    const auto button = qt::QtButtonToMouseButton(event->button());

    input_system_->GetMouse()->PressMouseButton(button);
    input_system_->GetMouse()->PressButton(glm::vec2{pos.x(), pos.y()}, button);
}
void RenderWindow::mouseMoveEvent(QMouseEvent* event) {
    // Touch input is handled in TouchUpdateEvent
    if (event->source() == Qt::MouseEventSynthesizedBySystem) {
        return;
    }
    // Qt sometimes returns the parent coordinates. To avoid this we read the global mouse
    // coordinates and map them to the current render area
    const auto pos = event->pos();
    const int center_x = width() / 2;
    const int center_y = height() / 2;

    input_system_->GetMouse()->MouseMove(glm::vec2{pos.x(), pos.y()});
    input_system_->GetMouse()->Move(pos.x(), pos.y(), center_x, center_y);
}
void RenderWindow::mouseReleaseEvent(QMouseEvent* event) {
    // Touch input is handled in TouchEndEvent
    if (event->source() == Qt::MouseEventSynthesizedBySystem) {
        return;
    }

    const auto button = qt::QtButtonToMouseButton(event->button());
    input_system_->GetMouse()->ReleaseButton(button);
}
void RenderWindow::wheelEvent(QWheelEvent* event) {
    const int x = event->angleDelta().x();
    const int y = event->angleDelta().y();
    input_system_->GetMouse()->Scroll(glm::vec2{x, y});
}

void RenderWindow::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);

    // windowHandle() is not initialized until the Window is shown, so we connect it here.
    connect(windowHandle(), &QWindow::screenChanged, this, &RenderWindow::OnFramebufferResized,
            Qt::UniqueConnection);
}
}  // namespace graphics