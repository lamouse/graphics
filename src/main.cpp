#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_sinks.h>
#include "g_app.hpp"

using namespace std;
using namespace g;

auto main(int /*argc*/, char** /*argv*/) -> int {
    // auto console = ::spdlog::stderr_logger_mt("console");
    // ::spdlog::set_default_logger(console);
    ::spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] %^[%l]%$ [thread %t] (%s:%# %!): %v");

#ifdef SPDLOG_ACTIVE_LEVEL
    spdlog::set_level(static_cast<spdlog::level::level_enum>(SPDLOG_ACTIVE_LEVEL));
#else
    #error "need set spd log level"
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
