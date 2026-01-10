
#include <utility>
#include "common/class_traits.hpp"
#include "common/thread.hpp"
#include <spdlog/spdlog.h>
#include "graphics/QT_render_widget.hpp"
#include "graphics/QT_window.hpp"
#include "graphics/QT_common.hpp"
#include "graphics/imgui_qt.hpp"
#include "input/mouse.h"
#include "input/keyboard.hpp"
#include "input/input.hpp"
#include "input/drop.hpp"
#include <QWindow>
#include <QLayout>
#include <QMimeData>

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

        ~RenderWidget() override = default;

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

RenderThread::RenderThread(core::System& sys, std::shared_ptr<input::InputSystem> input_system)
    : system(sys), input_system_(std::move(input_system)) {}

RenderThread::~RenderThread() = default;

void RenderThread::run() {
    common::thread::SetCurrentThreadName("RenderThread");
    auto stop_token = stop_source_.get_token();
    while (!stop_token.stop_requested()) {
        if (should_run_) {
            system.run(input_system_);
            should_run_.wait(false);
        } else {
            should_run_.wait(true);
        }
    }
}

void RenderThread::ForceStop() {
    spdlog::warn("Force stopping Render Thread");
    stop_source_.request_stop();
}

RenderWindow::RenderWindow(QTWindow* parent, std::shared_ptr<input::InputSystem> input_system)
    : QWidget(parent), input_system_(std::move(input_system)) {
    QWidget::setWindowTitle(QString("graphics"));
    setAttribute(Qt::WA_AcceptTouchEvents);
    setFocusPolicy(Qt::StrongFocus);
    auto* layout = new QHBoxLayout(this);  // NOLINT
    layout->setContentsMargins(0, 0, 0, 0);
    this->setMouseTracking(true);
    setAcceptDrops(true);
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

RenderWindow::~RenderWindow() { input_system_->Shutdown(); }

void RenderWindow::pullEvents() { QCoreApplication::processEvents(); }

void RenderWindow::newFrame() {
    QSize logicalSize = this->size();
    imgui::qt::new_frame(static_cast<float>(logicalSize.width()),
                         static_cast<float>(logicalSize.height()));
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
    child_widget_->setMinimumSize(1, 1);
    child_widget_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    window_info = qt::get_window_system_info(child_widget_->windowHandle());
    layout()->addWidget(child_widget_);
    setMinimumSize(1, 1);

    input_system_->Init();
    return true;
}
void RenderWindow::ReleaseRenderTarget() {
    if (child_widget_) {
        layout()->removeWidget(child_widget_);
        child_widget_->deleteLater();
        child_widget_ = nullptr;
    }
}

auto RenderWindow::initVulkanRenderWidget() -> bool {
    child_widget_ = new VulkanRenderWidget(this);  // NOLINT
    child_widget_->windowHandle()->create();
    if (!child_widget_) {
        return false;
    }
    child_widget_->setMouseTracking(true);
    return true;
}

void RenderWindow::OnFramebufferResized() {
    const qreal pixel_ratio = child_widget_->devicePixelRatio();
    const auto width = static_cast<std::uint32_t>(child_widget_->width() * pixel_ratio);
    const auto height = static_cast<std::uint32_t>(child_widget_->height() * pixel_ratio);
    QSize logicalSize = this->size();
    imgui::qt::new_frame(static_cast<float>(logicalSize.width()),
                         static_cast<float>(logicalSize.height()));
    UpdateCurrentFramebufferLayout(width, height);
}

void RenderWindow::mousePressEvent(QMouseEvent* event) {
    imgui::qt::mouse_press_event(event);
    // Touch input is handled in TouchBeginEvent
    if (event->source() == Qt::MouseEventSynthesizedBySystem) {
        return;
    }
    const auto pos = event->position();
    const auto button = qt::QtButtonToMouseButton(event->button());

    input_system_->GetMouse()->PressMouseButton(button);
    input_system_->GetMouse()->PressButton(glm::vec2{pos.x(), pos.y()}, button);

    QWidget::mousePressEvent(event);
}
void RenderWindow::mouseMoveEvent(QMouseEvent* event) {
    imgui::qt::mouse_move_event(event);
    // Touch input is handled in TouchUpdateEvent
    if (event->source() == Qt::MouseEventSynthesizedBySystem) {
        return;
    }
    // Qt sometimes returns the parent coordinates. To avoid this we read the global mouse
    // coordinates and map them to the current render area
    const auto pos = event->position();
    const int center_x = width() / 2;
    const int center_y = height() / 2;

    input_system_->GetMouse()->MouseMove(glm::vec2{pos.x(), pos.y()});
    input_system_->GetMouse()->Move(static_cast<int>(pos.x()), static_cast<int>(pos.y()), center_x,
                                    center_y);
}
void RenderWindow::mouseReleaseEvent(QMouseEvent* event) {
    imgui::qt::mouse_release_event(event);
    // Touch input is handled in TouchEndEvent
    if (event->source() == Qt::MouseEventSynthesizedBySystem) {
        return;
    }

    const auto button = qt::QtButtonToMouseButton(event->button());
    input_system_->GetMouse()->ReleaseButton(button);
}
void RenderWindow::wheelEvent(QWheelEvent* event) {
    imgui::qt::mouse_wheel_event(event);
    const int x = event->angleDelta().x();
    const int y = event->angleDelta().y() / 120;
    input_system_->GetMouse()->Scroll(glm::vec2{x, y});
}

void RenderWindow::focusInEvent(QFocusEvent* event) {
    imgui::qt::mouse_focus_in_event();
    QWidget::focusInEvent(event);
}
void RenderWindow::focusOutEvent(QFocusEvent* event) {
    imgui::qt::mouse_focus_out_event();
    QWidget::focusOutEvent(event);
    input_system_->GetMouse()->ResetButtonState();
    input_system_->GetKeyboard()->ReleaseAllKeys();
}

void RenderWindow::keyPressEvent(QKeyEvent* event) {
    auto modifiers = qt::KeyConvert(event->modifiers());
    input_system_->GetKeyboard()->SetKeyboardModifiers(modifiers);
    input_system_->GetKeyboard()->PressKey(qt::QtKeyToSwitchKey(Qt::Key(event->key())));
    imgui::qt::key_press(event);
}
void RenderWindow::keyReleaseEvent(QKeyEvent* event) {
    auto modifiers = qt::KeyConvert(event->modifiers());
    input_system_->GetKeyboard()->SetKeyboardModifiers(modifiers);
    input_system_->GetKeyboard()->ReleaseKey(qt::QtKeyToSwitchKey(Qt::Key(event->key())));
    imgui::qt::key_release(event);
}
void RenderWindow::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void RenderWindow::dropEvent(QDropEvent* event) {
    QList<QUrl> urls = event->mimeData()->urls();
    auto* file_drop = input_system_->GetFileDrop();
    for (const QUrl& url : urls) {
        QString filePath = url.toLocalFile();
        file_drop->push(filePath.toStdString());
    }
}

void RenderWindow::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);

    // windowHandle() is not initialized until the Window is shown, so we connect it here.
    connect(windowHandle(), &QWindow::screenChanged, this, &RenderWindow::OnFramebufferResized,
            Qt::UniqueConnection);
}
}  // namespace graphics