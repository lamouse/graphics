#pragma once

namespace sys {
class LoggerSystem;
}

namespace common::logger {
void init();
auto getLogger() -> sys::LoggerSystem*;

}  // namespace common::logger