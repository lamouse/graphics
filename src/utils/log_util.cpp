#include "log_util.hpp"
namespace utils {
auto get_log_level_from_string(const std::string& level) -> spdlog::level::level_enum {
    static const std::unordered_map<std::string, spdlog::level::level_enum> level_map = {
        {"trace", spdlog::level::trace}, {"debug", spdlog::level::debug},
        {"info", spdlog::level::info},   {"warn", spdlog::level::warn},
        {"error", spdlog::level::err},   {"critical", spdlog::level::critical},
        {"off", spdlog::level::off}};

    auto it = level_map.find(level);
    if (it != level_map.end()) {
        return it->second;
    }

    throw std::invalid_argument("Invalid log level: " + level);
}
spdlog::level::level_enum get_log_level(settings::enums::LogLevel level) {
    using Level = settings::enums::LogLevel;
    switch (level) {
        case Level::debug:
            return spdlog::level::debug;
        case Level::info:
            return spdlog::level::info;
        case Level::warn:
            return spdlog::level::warn;
        case Level::trace:
            return spdlog::level::trace;
        case Level::error:
            return spdlog::level::err;
        case Level::critical:
            return spdlog::level::critical;
        case Level::off:
            return spdlog::level::off;
        default:
            throw std::invalid_argument("Invalid log level: ");
    }
}
}  // namespace utils