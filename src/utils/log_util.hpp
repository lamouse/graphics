#pragma once
#include <spdlog/spdlog.h>
#include <string>
#include "common/settings_enums.hpp"

namespace utils {
spdlog::level::level_enum get_log_level_from_string(const std::string& level);
spdlog::level::level_enum get_log_level(settings::enums::LogLevel level);
}  // namespace utils