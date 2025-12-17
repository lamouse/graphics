#pragma once
#include "core/frontend/window.hpp"
#include "common/common_funcs.hpp"

#include <QMainWindow>

namespace graphics {

class QTWindow : public QMainWindow, public core::frontend::BaseWindow {
        Q_OBJECT
    public:
        QTWindow(int width, int height, ::std::string_view title);
        [[nodiscard]] auto IsShown() const -> bool override;
        [[nodiscard]] auto IsMinimized() const -> bool override;
        [[nodiscard]] auto shouldClose() const -> bool override;
        void OnFrameDisplayed() override {};
        [[nodiscard]] auto getActiveConfig() const -> WindowConfig override {
            return getWindowConfig();
        }
        void setWindowTitle(std::string_view title) override;

        void resizeEvent(QResizeEvent* event) override;

        void configGUI() override {};
        void destroyGUI() override {};
        void newFrame() override;
        void pullEvents(core::InputEvent& event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void wheelEvent(QWheelEvent* event) override;
        void setShouldClose() override;

    private:
        bool should_close_;
};

}  // namespace graphics