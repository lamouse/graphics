#pragma once
#include <QObject>
#include <spdlog/spdlog.h>
#include <QQmlApplicationEngine>
#include "common/settings.hpp"
namespace graphics {
class RenderController : public QObject {
        Q_OBJECT
    public:
        explicit RenderController(QObject* parent = nullptr)
            : QObject(parent), render_debug_(settings::values.render_debug.GetValue()) {}

    public slots:
        void handleOptionRenderDebug(bool checked) {
            spdlog::info("render debug: {}", checked);
            settings::values.render_debug.SetValue(checked);
        }
        Q_INVOKABLE auto isRenderDebugEnabled() const -> bool { return render_debug_; }

    private:
        bool render_debug_ = false;
};

void registerRenderController(QQmlApplicationEngine& engine);

}  // namespace graphics