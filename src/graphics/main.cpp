#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#ifdef USE_QT
#include <QScreen>
#include <QtQml/QQmlApplicationEngine>
#include <QWindow>
#include <QApplication>
#include <QFontDatabase>
#include "graphics/ui/render_config.hpp"
#endif
#if defined(_WIN32)
#include <windows.h>
#endif
#include "app.hpp"

using namespace std;
auto main([[maybe_unused]] int argc, [[maybe_unused]] char** argv) -> int {
#if defined(_WIN32)
    SetConsoleOutputCP(65001);
#endif
    try {
#ifdef USE_QT
        QApplication::setHighDpiScaleFactorRoundingPolicy(
            Qt::HighDpiScaleFactorRoundingPolicy::Floor);
        QApplication::addLibraryPath("plugins");
        QApplication app(argc, argv);

        // 加载中文字体
        int chineseId = QFontDatabase::addApplicationFont("fonts/AlibabaPuHuiTi-3-55-Regular.otf");
        QString chineseFamily = QFontDatabase::applicationFontFamilies(chineseId).at(0);

        // 加载图标字体（例如 FontAwesome）
        int iconId = QFontDatabase::addApplicationFont("fonts/MesloLGS NF Regular.ttf");
        QString iconFamily = QFontDatabase::applicationFontFamilies(iconId).at(0);

        // 设置全局默认字体为中文字体
        QApplication::setFont(QFont(chineseFamily, 10));

        // QQmlApplicationEngine engine;
        // graphics::registerRenderController(engine);
        // engine.addImportPath(QCoreApplication::applicationDirPath() + "/qml");
        // engine.addImportPath(":/");

        // QObject::connect(
        //     &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
        //     []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);
        // engine.loadFromModule("graphics", "Main");
        // if (engine.rootObjects().isEmpty()) {
        //     spdlog::warn("Failed to load QML root object.");
        // }

        // QApplication::exec();
#endif
        graphics::App g_app;
        g_app.run();
    } catch (const ::std::exception& e) {
        ::spdlog::error(e.what());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

#if defined(_WIN32)
auto WINAPI WinMain([[maybe_unused]] HINSTANCE hInstance, [[maybe_unused]] HINSTANCE hPrevInstance,
                    [[maybe_unused]] LPSTR lpCmdLine, [[maybe_unused]] int nCmdShow) -> int {
    return main(__argc, __argv);
}
#endif
