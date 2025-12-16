#pragma once
#include "core/frontend/window.hpp"
#include "common/common_funcs.hpp"

#include <QMainWindow>

namespace graphics {
class UIWindow : public QMainWindow {
        Q_OBJECT
    public:
        UIWindow(QWidget* parent = nullptr);
        void resizeEvent(QResizeEvent* event) override;
        CLASS_NON_COPYABLE(UIWindow);
        CLASS_NON_MOVEABLE(UIWindow);
        ~UIWindow() override;
};

class QTWindow : public core::frontend::BaseWindow, public QMainWindow {
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

        void configGUI() override {};
        void destroyGUI() override {};
        void newFrame() override {};
        void pullEvents(core::InputEvent& event) override;
        void setShouldClose() override;

    private:
        UIWindow window_;
        bool should_close_;
};

}  // namespace graphics