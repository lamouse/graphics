#include "graphics/QT_window.hpp"
#include "graphics/QT_common.hpp"
#include <Qscreen>
#include <QResizeEvent>
#include <imgui.h>
namespace graphics {

QTWindow::QTWindow(int width, int height, ::std::string_view title) : should_close_(false) {
    core::frontend::BaseWindow::WindowConfig conf;
    conf.extent.width = width;
    conf.extent.height = height;
    conf.fullscreen = false;
    setWindowConfig(conf);
    QScreen* screen = QGuiApplication::primaryScreen();
    qreal scale = screen->devicePixelRatio();
    this->resize(static_cast<int>(width / scale), static_cast<int>(height / scale));
    QMainWindow::setWindowTitle(QString::fromStdString(std::string(title)));
#if defined(SDL_PLATFORM_MACOS)
    window_info.render_surface_scale = 1.0f;
#else
    window_info.render_surface_scale = 1.f;
#endif
    window_info.type = qt::get_window_system_info();
    window_info.render_surface = reinterpret_cast<void*>(this->winId());
    this->show();
}
auto QTWindow::IsShown() const -> bool { return this->isVisible(); }

auto QTWindow::IsMinimized() const -> bool { return this->isMinimized(); }

auto QTWindow::shouldClose() const -> bool { return should_close_ || !this->isVisible(); }
void QTWindow::setWindowTitle(std::string_view title) {
    QMainWindow::setWindowTitle(QString::fromStdString(std::string(title)));
}

void QTWindow::pullEvents(core::InputEvent& event) { QCoreApplication::processEvents(); }
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
    QSize physicalSize = logicalSize * this->devicePixelRatio();
    QScreen* screen = QGuiApplication::primaryScreen();
    auto geometry = screen->geometry();
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize =
        ImVec2(static_cast<float>(logicalSize.width()), static_cast<float>(logicalSize.height()));
}

void QTWindow::mousePressEvent(QMouseEvent* event) {
    ImGuiIO& io = ImGui::GetIO();
    int mouse_button = -1;
    switch (event->button()) {
        case Qt::LeftButton:
            mouse_button = 0;
            break;
        case Qt::RightButton:
            mouse_button = 1;
            break;
        case Qt::MiddleButton:
            mouse_button = 2;
            break;
        default:
            break;
    }
    QMainWindow::mousePressEvent(event);
    if (mouse_button == -1) {
        return;
    }
    io.MousePos = ImVec2(event->position().x(), event->position().y());
    io.AddMouseButtonEvent(mouse_button, true);
}

void QTWindow::mouseReleaseEvent(QMouseEvent* event) {
    int mouse_button = -1;
    ImGuiIO& io = ImGui::GetIO();
    if (event->button() == Qt::LeftButton) {
        mouse_button = 0;
    }
    if (event->button() == Qt::RightButton) {
        mouse_button = 1;
    }
    if (event->button() == Qt::MiddleButton) {
        mouse_button = 2;
    }

    QMainWindow::mouseReleaseEvent(event);
    if (mouse_button == -1) {
        return;
    }
    io.MousePos = ImVec2(event->position().x(), event->position().y());
    io.AddMouseButtonEvent(mouse_button, false);
}

void QTWindow::mouseMoveEvent(QMouseEvent* event) {
    ImGuiIO& io = ImGui::GetIO();
    io.MousePos = ImVec2(event->position().x(), event->position().y());
    QMainWindow::mouseMoveEvent(event);
}

void QTWindow::wheelEvent(QWheelEvent* event) {
    ImGuiIO& io = ImGui::GetIO();
    io.MouseWheel += event->angleDelta().y() / 120.0f;   // 垂直滚轮
    io.MouseWheelH += event->angleDelta().x() / 120.0f;  // 水平滚轮
}
}  // namespace graphics