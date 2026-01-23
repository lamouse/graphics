#include "common/logger.hpp"
#include <spdlog/spdlog.h>
#ifdef USE_QT
#include <QScreen>
#include <QtQml/QQmlApplicationEngine>
#include <QWindow>
#include <QApplication>
#include <QFontDatabase>
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
        common::logger::init();
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

#endif
        graphics::App g_app;

#ifdef USE_QT
        QApplication::exec();
#endif
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
