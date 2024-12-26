#pragma once
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/spdlog.h>

#include <stdexcept>
#include <string>
#include <unordered_map>
namespace utils {
spdlog::level::level_enum get_log_level_from_string(const std::string& level);
}