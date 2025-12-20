#pragma once
#include <QObject>
#include <spdlog/spdlog.h>
#include <QtQml/QQmlApplicationEngine>
#include "common/settings.hpp"
namespace graphics {
class RenderController : public QObject {
        Q_OBJECT
        QML_ELEMENT
    public:
        explicit RenderController(QObject* parent = nullptr)
            : QObject(parent),
              render_debug_(settings::values.render_debug.GetValue()),
              use_debug_ui_(settings::values.use_debug_ui.GetValue()) {}

    public slots:
        void handleOptionRenderDebug(bool checked) {
            spdlog::info("render debug: {}", checked);
            settings::values.render_debug.SetValue(checked);
        }
        Q_INVOKABLE auto isRenderDebugEnabled() const -> bool { return render_debug_; }

        Q_INVOKABLE void setUseDebugUI(bool enabled) {
            spdlog::info("use debug ui: {}", enabled);
            settings::values.use_debug_ui.SetValue(enabled);
            use_debug_ui_ = enabled;
        }
        Q_INVOKABLE auto isUseDebugUIEnabled() const -> bool { return use_debug_ui_; }

    private:
        bool render_debug_ = false;
        bool use_debug_ui_{false};
};

void registerRenderController(QQmlApplicationEngine& engine);

}  // namespace graphics