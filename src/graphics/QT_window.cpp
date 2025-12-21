#include "graphics/QT_window.hpp"
#include "graphics/QT_common.hpp"
#include "graphics/QT_render_widget.hpp"
#include "common/settings.hpp"
#include <Qscreen>
#include <QResizeEvent>
#include <QFileDialog>
#include <QMenuBar>
#include <QAction>
#include <spdlog/spdlog.h>
#include <imgui.h>


namespace graphics {

QTWindow::QTWindow(int width, int height, ::std::string_view title) : should_close_(false) {
    core::frontend::BaseWindow::WindowConfig conf;
    conf.extent.width = width;
    conf.extent.height = height;
    conf.fullscreen = false;
    setWindowConfig(conf);
    this->resize(static_cast<int>(width), static_cast<int>(height));
    QMainWindow::setWindowTitle(QString::fromStdString(std::string(title)));
    qreal scale = 1.f;
#ifdef _WIN32
    scale = this->screen()->logicalDotsPerInch() / 96.0;
#endif

    window_info.render_surface_scale = static_cast<float>(scale);
    window_info.type = qt::get_window_system_info();
    window_info.render_surface = reinterpret_cast<void*>(this->winId());
    this->setMouseTracking(true);
    auto* fileMenu = menuBar()->addMenu(tr("&File"));
    auto* openAction = new QAction(tr("&Open"), this);
    fileMenu->addAction(openAction);
    connect(openAction, &QAction::triggered, this, &QTWindow::openFile);
    openAction->setShortcut(QKeySequence::Open);

    auto* viewMenu = menuBar()->addMenu(tr("&View"));
    auto* debugUIAction = new QAction(tr("Toggle &Debug UI"), this);
    viewMenu->addAction(debugUIAction);
    connect(debugUIAction, &QAction::triggered, this, &QTWindow::setDebugUI);
    debugUIAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_D));

    this->show();
}
auto QTWindow::IsShown() const -> bool { return this->isVisible(); }

auto QTWindow::IsMinimized() const -> bool {
    return this->isMinimized() || this->isHidden() || !this->IsShown();
}

auto QTWindow::shouldClose() const -> bool { return should_close_ || !this->isVisible(); }
void QTWindow::setWindowTitle(std::string_view title) {
    QMainWindow::setWindowTitle(QString::fromStdString(std::string(title)));
}

void QTWindow::pullEvents(core::InputEvent& event) {
    QCoreApplication::processEvents();

    while (!eventQueue.empty()) {
        event.push_event(eventQueue.front());
        eventQueue.pop();
    }
}
void QTWindow::setShouldClose() {
    should_close_ = true;
    this->close();
}

void QTWindow::resizeEvent(QResizeEvent* event) {
    QSize newSize = event->size();
    this->setWindowConfig(WindowConfig{.extent = {.width = static_cast<int>(newSize.width()),
                                                  .height = static_cast<int>(newSize.height())}});
    QMainWindow::resizeEvent(event);  // 调用基类保证正常行为
}
void QTWindow::newFrame() {
    QSize logicalSize = this->size();
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize =
        ImVec2(static_cast<float>(logicalSize.width()), static_cast<float>(logicalSize.height()));
}

void QTWindow::mousePressEvent(QMouseEvent* event) {
    ImGuiIO& io = ImGui::GetIO();
    int mouse_button = -1;
    core::InputState input_state;
    input_state.key_down.Assign(1);
    input_state.first_down.Assign(1);
    switch (event->button()) {
        case Qt::LeftButton:
            mouse_button = 0;
            input_state.mouse_button_left.Assign(1);
            break;
        case Qt::RightButton:
            mouse_button = 1;
            input_state.mouse_button_center.Assign(1);
            break;
        case Qt::MiddleButton:
            mouse_button = 2;
            input_state.mouse_button_right.Assign(1);
            break;
        default:
            break;
    }
    QMainWindow::mousePressEvent(event);
    if (mouse_button == -1) {
        return;
    }
    input_state.mouseX_ = event->position().x();
    input_state.mouseY_ = event->position().y();
    eventQueue.push(input_state);
    io.MousePos = ImVec2(event->position().x(), event->position().y());
    auto source = event->source();
    if (source == Qt::MouseEventNotSynthesized) {
        io.AddMouseSourceEvent(ImGuiMouseSource_Mouse);
    } else if (source == Qt::MouseEventSynthesizedBySystem) {
        io.AddMouseSourceEvent(ImGuiMouseSource_TouchScreen);
    }
    io.AddMouseButtonEvent(mouse_button, true);
}

void QTWindow::mouseReleaseEvent(QMouseEvent* event) {
    int mouse_button = -1;
    ImGuiIO& io = ImGui::GetIO();
    core::InputState input_state;
    input_state.key_up.Assign(1);
    if (event->button() == Qt::LeftButton) {
        mouse_button = 0;
        input_state.mouse_button_left.Assign(1);
    }
    if (event->button() == Qt::RightButton) {
        mouse_button = 1;
        input_state.mouse_button_center.Assign(1);
    }
    if (event->button() == Qt::MiddleButton) {
        mouse_button = 2;
        input_state.mouse_button_right.Assign(1);
    }

    QMainWindow::mouseReleaseEvent(event);
    if (mouse_button == -1) {
        return;
    }

    input_state.key_up.Assign(1);
    input_state.mouseX_ = event->position().x();
    input_state.mouseY_ = event->position().y();
    eventQueue.push(input_state);

    io.MousePos = ImVec2(event->position().x(), event->position().y());
    auto source = event->source();
    if (source == Qt::MouseEventNotSynthesized) {
        io.AddMouseSourceEvent(ImGuiMouseSource_Mouse);
    } else if (source == Qt::MouseEventSynthesizedBySystem) {
        io.AddMouseSourceEvent(ImGuiMouseSource_TouchScreen);
    }
    io.AddMouseButtonEvent(mouse_button, false);
}

void QTWindow::mouseMoveEvent(QMouseEvent* event) {
    ImGuiIO& io = ImGui::GetIO();
    io.AddMousePosEvent(event->position().x(), event->position().y());
    core::InputState input_state;
    input_state.mouseX_ = event->position().x();
    input_state.mouseY_ = event->position().y();
    input_state.mouse_move.Assign(1);
    Qt::MouseButtons buttons = event->buttons();
    if (buttons & Qt::LeftButton) {
        input_state.mouse_button_left.Assign(1);
        input_state.key_down.Assign(1);
    }
    if (buttons & Qt::RightButton) {
        input_state.mouse_button_right.Assign(1);
        input_state.key_down.Assign(1);
    }
    if (buttons & Qt::MiddleButton) {
        input_state.mouse_button_center.Assign(1);
        input_state.key_down.Assign(1);
    }
    if (lastMouseX_ >= 0 && lastMouseY_ >= 0) {
        input_state.mouseRelativeX_ = event->position().x() - lastMouseX_;
        input_state.mouseRelativeY_ = event->position().y() - lastMouseY_;
    }
    lastMouseX_ = event->position().x();
    lastMouseY_ = event->position().y();
    eventQueue.push(input_state);
    QMainWindow::mouseMoveEvent(event);
}

void QTWindow::wheelEvent(QWheelEvent* event) {
    ImGuiIO& io = ImGui::GetIO();
    io.MouseWheel += event->angleDelta().y() / 120.0f;   // 垂直滚轮
    io.MouseWheelH += event->angleDelta().x() / 120.0f;  // 水平滚轮
    auto source = event->source();
    if (source == Qt::MouseEventNotSynthesized) {
        io.AddMouseSourceEvent(ImGuiMouseSource_Mouse);
    } else if (source == Qt::MouseEventSynthesizedBySystem) {
        io.AddMouseSourceEvent(ImGuiMouseSource_TouchScreen);
    }
    core::InputState input_state;
    input_state.mouseX_ = event->position().x();
    input_state.mouseY_ = event->position().y();
    input_state.scrollOffset_ = event->angleDelta().y() / 120.0f;
    eventQueue.push(input_state);
}
void QTWindow::focusInEvent(QFocusEvent* event) {
    ImGuiIO& io = ImGui::GetIO();
    io.AddFocusEvent(true);
    QMainWindow::focusInEvent(event);
}
void QTWindow::focusOutEvent(QFocusEvent* event) {
    ImGuiIO& io = ImGui::GetIO();
    io.AddFocusEvent(false);
    QMainWindow::focusOutEvent(event);
}
void QTWindow::openFile(){
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), "", tr("All Files (*)"));
    if (!fileName.isEmpty()) {
        spdlog::info("Selected file: {}", fileName.toStdString());
    }
}
void QTWindow::setDebugUI(){
    settings::values.use_debug_ui.SetValue(!settings::values.use_debug_ui.GetValue());
}
}  // namespace graphics