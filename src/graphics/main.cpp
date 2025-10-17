#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#if defined(_WIN32)
#include <windows.h>
#endif
#include "resource/config.hpp"
#include "app.hpp"



using namespace std;
using namespace g;
auto main(int /*argc*/, char** /*argv*/) -> int {
#if defined(_WIN32)
    SetConsoleOutputCP(65001);
#endif
    try {
        const Config config("config/config.yaml");
        graphics::App app(config);
        app.run();

    } catch (const ::std::exception& e) {
        ::spdlog::error(e.what());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

#if defined (_WIN32)
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    return main(1, nullptr);
}
#endif
