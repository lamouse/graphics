#include "graphics/ui/render_config.hpp"
#include <QAction>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlContext>
namespace graphics {
void registerRenderController(QQmlApplicationEngine& engine) {
    auto* ctrl = new RenderController(&engine);  // NOLINT
    engine.rootContext()->setContextProperty("render_ctrl", ctrl);
}

}  // namespace graphics