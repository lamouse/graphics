#include "common/settings.hpp"
#include <spdlog/spdlog.h>
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
        settings::load_settings();
        graphics::App g_app;
        settings::save_settings();
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
