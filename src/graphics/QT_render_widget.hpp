#pragma once
#include <QWidget>
#include "core/frontend/window.hpp"
#include <memory>
namespace graphics {
namespace input {
class InputSystem;
}
class QTWindow;

class RenderWindow : public QWidget, public core::frontend::BaseWindow {
        Q_OBJECT
    public:
        explicit RenderWindow(QTWindow* parent, std::shared_ptr<input::InputSystem> input_system);

        void Exit();
        void ExecuteProgram(std::size_t program_index);
        [[nodiscard]] auto IsLoadingComplete() const -> bool;
        [[nodiscard]] auto IsShown() const -> bool override;
        [[nodiscard]] auto IsMinimized() const -> bool override;
        [[nodiscard]] auto shouldClose() const -> bool override;
        void setWindowTitle(std::string_view title) override {};
        void setShouldClose() override {
            should_close_ = true;
            this->close();
        };
        void configGUI() override {};
        void destroyGUI() override {};
        void pullEvents(core::InputEvent& event) override {}
        void newFrame() override;
        void OnFrameDisplayed() override;
        [[nodiscard]] auto getActiveConfig() const -> WindowConfig override {
            return getWindowConfig();
        }

        void closeEvent(QCloseEvent* event) override;
        void leaveEvent(QEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;

        void mousePressEvent(QMouseEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void wheelEvent(QWheelEvent* event) override;
        void focusInEvent(QFocusEvent* event) override;
        void focusOutEvent(QFocusEvent* event) override;

        [[nodiscard]] auto windowPixelRatio() const -> qreal;
        auto InitRenderTarget() -> bool;
        /// Destroy the previous run's child_widget which should also destroy the child_window
        void ReleaseRenderTarget();
        void BackupGeometry();
        // NOLINTNEXTLINE
    public slots:
        void OnFramebufferResized();
    signals:
        void CLosed();
        void ExitSignal();
        void ExecuteProgramSignal(std::size_t program_index);
        void FirstFrameDisplayed();

    private:
        auto initVulkanRenderWidget() -> bool;

        bool should_close_{false};
        bool first_frame{false};
        std::shared_ptr<input::InputSystem> input_system_;
        QWidget* child_widget_ = nullptr;

    protected:
        void showEvent(QShowEvent* event) override;
};

}  // namespace graphics