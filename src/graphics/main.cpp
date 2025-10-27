#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#if defined(_WIN32)
#include <windows.h>
#endif
#include "app.hpp"



using namespace std;
auto main(int /*argc*/, char** /*argv*/) -> int {
#if defined(_WIN32)
    SetConsoleOutputCP(65001);
#endif
    try {
        graphics::App app;
        app.run();

    } catch (const ::std::exception& e) {
        ::spdlog::error(e.what());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

#if defined (_WIN32)
auto WINAPI WinMain([[maybe_unused]]HINSTANCE hInstance, [[maybe_unused]]HINSTANCE hPrevInstance, [[maybe_unused]]LPSTR lpCmdLine, [[maybe_unused]]int nCmdShow)->int {
    return main(1, nullptr);
}
#endif
