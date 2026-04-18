#pragma once
#include <spdlog/spdlog.h>
#define LOG_ERROR(MODULE, TAG, ...) SPDLOG_ERROR("[{}][{}] " __VA_ARGS__, MODULE, TAG)

namespace sys {
class LoggerSystem;
}

namespace common::logger {
void init();
auto getLogger() -> sys::LoggerSystem*;

}  // namespace common::logger