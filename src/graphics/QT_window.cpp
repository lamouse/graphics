#include "graphics/QT_window.hpp"
#include "graphics/QT_common.hpp"
#include <Qscreen>
#include <QResizeEvent>
namespace graphics {

UIWindow::UIWindow(QWidget* parent) : QMainWindow(parent) {}
void UIWindow::resizeEvent(QResizeEvent* event) {
    QSize newSize = event->size();
    QMainWindow::resizeEvent(event);  // 调用基类保证正常行为
}
UIWindow::~UIWindow() = default;

QTWindow::QTWindow(int width, int height, ::std::string_view title) : should_close_(false) {
    core::frontend::BaseWindow::WindowConfig conf;
    conf.extent.width = width;
    conf.extent.height = height;
    conf.fullscreen = false;
    setWindowConfig(conf);

    window_.resize(width, height);
    window_.setWindowTitle(QString::fromStdString(std::string(title)));
#if defined(SDL_PLATFORM_MACOS)
    window_info.render_surface_scale = 1.0f;
#else
    window_info.render_surface_scale = static_cast<float>(window_.screen()->devicePixelRatio());
#endif
    window_info.type = qt::get_window_system_info();
    window_info.render_surface = reinterpret_cast<void*>(window_.winId());
    window_.show();
}
auto QTWindow::IsShown() const -> bool { return window_.isVisible(); }

auto QTWindow::IsMinimized() const -> bool { return window_.isMinimized(); }

auto QTWindow::shouldClose() const -> bool { return should_close_ || !window_.isVisible(); }
void QTWindow::setWindowTitle(std::string_view title) {
    window_.setWindowTitle(QString::fromStdString(std::string(title)));
}

void QTWindow::pullEvents(core::InputEvent& event) { QCoreApplication::processEvents(); }
void QTWindow::setShouldClose() {
    should_close_ = true;
    this->close();
}
}  // namespace graphics