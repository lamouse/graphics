#pragma once
#include <QObject>
#include <spdlog/spdlog.h>
#include <QtQml/QQmlApplicationEngine>
#include <QAbstractListModel>
#include "common/settings.hpp"
namespace graphics {
class RenderController : public QObject {
        Q_OBJECT
        QML_ELEMENT
    public:
        explicit RenderController(QObject *parent = nullptr)
            : QObject(parent),
              render_debug_(settings::values.render_debug.GetValue()),
              use_debug_ui_(settings::values.use_debug_ui.GetValue()) {}

    // NOLINTNEXTLINE
    public slots:
        void handleOptionRenderDebug(bool checked) {
            spdlog::info("render debug: {}", checked);
            settings::values.render_debug.SetValue(checked);
        }
        Q_INVOKABLE [[nodiscard]] auto isRenderDebugEnabled() const -> bool {
            return render_debug_;
        }

        Q_INVOKABLE void setUseDebugUI(bool enabled) {
            spdlog::info("use debug ui: {}", enabled);
            settings::values.use_debug_ui.SetValue(enabled);
            use_debug_ui_ = enabled;
        }
        Q_INVOKABLE [[nodiscard]] auto isUseDebugUIEnabled() const -> bool { return use_debug_ui_; }

    private:
        bool render_debug_ = false;
        bool use_debug_ui_{false};
};

class ResolutionModel : public QAbstractListModel {
        Q_OBJECT
        QML_ELEMENT
    public:
        enum Roles { TextRole = Qt::UserRole + 1 };
        ResolutionModel(QObject *parent = nullptr) : QAbstractListModel(parent) {
            items = {QString("%1/%2").arg(settings::values.resolution.weight).arg(settings::values.resolution.height)};
        }
        [[nodiscard]] auto rowCount(const QModelIndex &parent = QModelIndex()) const
            -> int override {
            return items.size();
        }
        [[nodiscard]] auto data(const QModelIndex &index, int role) const -> QVariant override {
            if (!index.isValid()) {
                return {};
            }
            if (role == TextRole) {
                return items.at(index.row());
            }
            return {};
        }
        [[nodiscard]] auto roleNames() const -> QHash<int, QByteArray> override {
            return {{TextRole, "text"}};
        }
    // NOLINTNEXTLINE
    public slots:
        void onItemSelected(int index) {
            if (index >= 0 && index < items.size()) {
                qDebug() << "选中分辨率:" << items.at(index);
            }
        }

    private:
        QStringList items;
};

void registerRenderController(QQmlApplicationEngine &engine);

}  // namespace graphics