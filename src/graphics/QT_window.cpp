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

QTWindow::QTWindow(std::shared_ptr<input::InputSystem> input_system,
                   std::shared_ptr<core::System>& system, int width, int height,
                   ::std::string_view title)
    : input_system_(std::move(input_system)), system_(std::make_shared<core::System>()) {
    // system = system_;
    input_system_->Init();
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

    auto* runMenu = menuBar()->addMenu(tr("&Run"));
    auto* run_start_action = new QAction(tr("Run"), this);
    runMenu->addAction(run_start_action);
    connect(run_start_action, &QAction::triggered, this, &QTWindow::StartRender);
    run_start_action->setShortcut(Qt::Key_F5);

    auto* render_stop_action = new QAction(tr("Stop"), this);
    runMenu->addAction(render_stop_action);
    connect(render_stop_action, &QAction::triggered, this, &QTWindow::EndRender);

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
    if (render_thread) {
        EndRender();
    }
    render_window_->close();
    if(engine_){
        engine_->exit(0);
    }
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
void QTWindow::StartRender() {
    if (render_thread && render_thread->IsRunning()) {
        return;
    }
    system_->setShutdown(false);
    auto* render_window = initRenderWindow();
    system_->load(*render_window);
    render_thread = std::make_unique<RenderThread>(*system_, input_system_);
    render_thread->SetRunning(true);
    render_thread->start();
    render_running_ = true;
}
void QTWindow::EndRender() {
    OnShutdownBegin();
    OnEndRenderStopTimeExpired();
    QTWindow::OnRenderStopped();
}

auto QTWindow::OnShutdownBegin() -> bool {
    if(!render_running_){
        return false;
    }

    if(system_->isShutdown()){
        return false;
    }

    system_->setShutdown(true);
    render_thread->disconnect();
    render_thread->SetRunning(true);
    int close_time{1000};
    stop_timer_.setSingleShot(true);
    stop_timer_.start(close_time);
    connect(&stop_timer_, &QTimer::timeout, this, &QTWindow::OnEndRenderStopTimeExpired);
    connect(render_thread.get(), &QThread::finished, this, &QTWindow::OnRenderStopped);
    return true;
}
void QTWindow::OnEndRenderStopTimeExpired() {
    if (render_thread) {
        render_thread->ForceStop();
    }
}
void QTWindow::OnRenderStopped() {
    stop_timer_.stop();
    if (render_thread) {
        render_thread->disconnect();
        render_thread->wait();
        render_thread.reset();
    }
    render_window_->hide();
}
}  // namespace graphics