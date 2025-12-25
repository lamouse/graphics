#include "core/frontend/window.hpp"
#include "common/class_traits.hpp"
#include "core/core.hpp"
#include <memory>
#include <QWidget>
#include <QThread>
#include <stop_token>
#include <atomic>

namespace graphics {
namespace input {
class InputSystem;
}

class RenderThread final : public QThread {
        Q_OBJECT

    public:
        explicit RenderThread(core::System& sys, std::shared_ptr<input::InputSystem> input_system);
        ~RenderThread() override;
        CLASS_NON_COPYABLE(RenderThread);
        CLASS_NON_MOVEABLE(RenderThread);
        void run() override;

        void SetRunning(bool running) {
            should_run_.store(running, std::memory_order::release);
            should_run_.notify_one();
        }

        auto IsRunning() -> bool {
            return should_run_;
        }
        void ForceStop();

    private:
        core::System& system;
        std::shared_ptr<input::InputSystem> input_system_;
        std::stop_source stop_source_;
        std::atomic<bool> should_run_{true};
};


class QTWindow;

class RenderWindow : public QWidget, public core::frontend::BaseWindow {
        Q_OBJECT
    public:
        explicit RenderWindow(QTWindow* parent, std::shared_ptr<input::InputSystem> input_system);
        CLASS_NON_COPYABLE(RenderWindow);
        CLASS_NON_MOVEABLE(RenderWindow);
        ~RenderWindow() override;
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
        void pullEvents() override;
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