#include "graphics/ui/render_config.hpp"
#include <QAction>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlContext>
namespace graphics {
void registerRenderController(QQmlApplicationEngine& engine) {
    // auto* ctrl = new RenderController(&engine);  // NOLINT
    // engine.rootContext()->setContextProperty("render_ctrl", ctrl);

    // auto* resolution = new ResolutionModel();// NOLINT
    // engine.rootContext()->setContextProperty("resolution_ctrl", resolution);

    // qmlRegisterType<ResolutionModel>("Graphics", 1, 0, "ResolutionModel");
}

}  // namespace graphics