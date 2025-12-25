#pragma once
#include <QMainWindow>
#include <QTimer>
#include <memory>
#include "core/frontend/window.hpp"
#include "common/class_traits.hpp"
#include "core/core.hpp"
class QQmlApplicationEngine;
namespace graphics {
class RenderThread;
namespace input {
class InputSystem;

}
class RenderWindow;

class QTWindow : public QMainWindow {
        Q_OBJECT
    public:
        CLASS_NON_COPYABLE(QTWindow);
        CLASS_NON_MOVEABLE(QTWindow);
        QTWindow(std::shared_ptr<input::InputSystem> input_system,
                 std::shared_ptr<core::System>& system, int width, int height,
                 ::std::string_view title);

        auto initRenderWindow() -> core::frontend::BaseWindow*;
        ~QTWindow() override;
        void openRenderConfig();
        void openFile();
        void setDebugUI();
        void closeEvent(QCloseEvent* event) override;

        // NOLINTNEXTLINE
    public slots:
        void OnExit();
        void OnExecuteProgram(std::size_t program_index);
        void OnLoadComplete();

    private:
        RenderWindow* render_window_{nullptr};
        void InitializeWidgets();

        void StartRender();
        void EndRender();
        QQmlApplicationEngine* engine_{nullptr};
        std::shared_ptr<input::InputSystem> input_system_;
        std::shared_ptr<core::System> system_;
        std::unique_ptr<RenderThread> render_thread;

        QTimer stop_timer_;
        bool render_running_{false};

        // NOLINTNEXTLINE
    private slots:
        auto OnShutdownBegin() -> bool;
        void OnRenderStopped();
        void OnEndRenderStopTimeExpired();
};

}  // namespace graphics