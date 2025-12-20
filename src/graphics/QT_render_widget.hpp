#pragma once
#include <QWidget>
namespace graphics {
class RenderWidget : public QWidget {
        Q_OBJECT
    public:
        explicit RenderWidget(QWidget* parent = nullptr) : QWidget(parent) {}
};

}  // namespace graphics