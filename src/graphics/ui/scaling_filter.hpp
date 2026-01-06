#pragma once
#include <QObject>
#include <spdlog/spdlog.h>
#include <QtQml/QQmlApplicationEngine>
#include <QAbstractListModel>
#include "common/settings.hpp"
namespace graphics {

class ScalingFilterModel : public QAbstractListModel {
        Q_OBJECT
        QML_ELEMENT
    public:
        enum Roles { TextRole = Qt::UserRole + 1 };
        ScalingFilterModel(QObject *parent = nullptr) : QAbstractListModel(parent) {
            auto current_filter = settings::values.scaling_filter.GetValue();
            items.push_back(
                QString::fromStdString(settings::enums::CanonicalizeEnum(current_filter)));
            auto scaling_filter =
                settings::enums::EnumMetadata<settings::enums::ScalingFilter>::canonicalizations();
            for (const auto &[name, value] : scaling_filter) {
                if (value == current_filter) {
                    continue;
                }
                items.push_back(QString::fromStdString(name));
            }
        }
        [[nodiscard]] auto rowCount(const QModelIndex &parent = QModelIndex()) const
            -> int override {
            return static_cast<int>(items.size());
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
                qDebug() << "filter:" << items.at(index);
                auto enum_value = settings::enums::ToEnum<settings::enums::ScalingFilter>(
                    items.at(index).toStdString());
                settings::values.scaling_filter.SetValue(enum_value);
            }
        }

    private:
        QStringList items;
};

}  // namespace graphics