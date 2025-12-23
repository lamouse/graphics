#pragma once
#include <QMainWindow>
#include <memory>
#include "core/frontend/window.hpp"
#include "common/common_funcs.hpp"
class QQmlApplicationEngine;

namespace graphics {
namespace input {
class InputSystem;

}
class RenderWindow;

class QTWindow : public QMainWindow {
        Q_OBJECT
    public:
        CLASS_NON_COPYABLE(QTWindow);
        CLASS_NON_MOVEABLE(QTWindow);
        QTWindow(std::shared_ptr<input::InputSystem> input_system, int width, int height,
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
        QQmlApplicationEngine* engine_{nullptr};
        std::shared_ptr<input::InputSystem> input_system_;
};

}  // namespace graphics