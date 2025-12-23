#include "graphics/QT_window.hpp"
#include "graphics/QT_render_widget.hpp"
#include "graphics/ui/render_config.hpp"
#include "input/input.hpp"
#include "common/settings.hpp"
#include <Qscreen>
#include <QResizeEvent>
#include <QFileDialog>
#include <QMenuBar>
#include <QAction>
#include <QtQml/QQmlApplicationEngine>

#include <QtQuick/QQuickWindow>
#include <utility>
#include <spdlog/spdlog.h>
#include <imgui.h>

namespace graphics {

QTWindow::QTWindow(std::shared_ptr<input::InputSystem> input_system, int width, int height,
                   ::std::string_view title)
    : input_system_(std::move(input_system)) {
    core::frontend::BaseWindow::WindowConfig conf;
    conf.extent.width = width;
    conf.extent.height = height;
    conf.fullscreen = false;
    this->resize(static_cast<int>(width), static_cast<int>(height));
    QMainWindow::setWindowTitle(QString::fromStdString(std::string(title)));

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
    debugUIAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_D));

    auto* configMenu = menuBar()->addMenu(tr("&Config"));
    auto* configAction = new QAction(tr("Toggle &config"), this);
    configMenu->addAction(configAction);
    connect(configAction, &QAction::triggered, this, &QTWindow::openRenderConfig);
    debugUIAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_C));
    // this->windowHandle()->create();

    this->show();
    InitializeWidgets();
}
void QTWindow::OnExit() { spdlog::debug("Received exit signal from render widget."); }

void QTWindow::OnExecuteProgram(std::size_t program_index) {
    spdlog::debug("on execute program {}", program_index);
}
void QTWindow::OnLoadComplete() { spdlog::debug("load complete"); }

void QTWindow::openFile() {
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), "", tr("All Files (*)"));
    if (!fileName.isEmpty()) {
        spdlog::info("Selected file: {}", fileName.toStdString());
    }
}
void QTWindow::setDebugUI() {
    settings::values.use_debug_ui.SetValue(!settings::values.use_debug_ui.GetValue());
}

void QTWindow::openRenderConfig() {
    if (engine_ == nullptr) {
        engine_ = new QQmlApplicationEngine(this);  // NOLINT
        registerRenderController(*engine_);
        engine_->addImportPath(QCoreApplication::applicationDirPath() + "/qml");
        engine_->addImportPath(":/");
        engine_->loadFromModule("RenderCtrl", "RenderControl");
        if (engine_->rootObjects().isEmpty()) {
            spdlog::warn("Failed to load RenderConfig.qml");
            return;
        }
    }

    auto* qmlWindow = qobject_cast<QQuickWindow*>(engine_->rootObjects().first());
    if (!qmlWindow) {
        spdlog::warn("Root object is not a QQuickWindow");
        return;
    }
    qmlWindow->show();
    // QWidget* widget = QWidget::createWindowContainer(qmlWindow, this);
    // widget->setWindowTitle(tr("Render Configuration"));
    // widget->resize(400, 300); // 必须设置大小
    // widget->show();
}

QTWindow::~QTWindow() {
    if (render_window_ && !render_window_->parent()) {
        delete render_window_;
    }
}

void QTWindow::closeEvent(QCloseEvent* event) {
    render_window_->close();
    QWidget::closeEvent(event);
}

void QTWindow::InitializeWidgets() {
    render_window_ = new RenderWindow(this, input_system_);  // NOLINT
    render_window_->hide();
}
auto QTWindow::initRenderWindow() -> core::frontend::BaseWindow* {
    render_window_->InitRenderTarget();
    this->setCentralWidget(render_window_);
    render_window_->show();
    return render_window_;
}
}  // namespace graphics