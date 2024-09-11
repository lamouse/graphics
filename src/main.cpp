
#include <spdlog/spdlog.h>

#include "g_app.hpp"

using namespace std;
using namespace g;

auto main(int /*argc*/, char** /*argv*/) -> int {
#ifdef NO_DEBUG
    ::spdlog::set_level(::spdlog::level::info);
#else
    ::spdlog::set_level(::spdlog::level::debug);
#endif
    g::App app;
    try {
        app.run();
    } catch (const ::std::exception& e) {
        ::spdlog::error(e.what());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
