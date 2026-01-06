#include "common/logger.hpp"
#include "system/logger_system.hpp"
#include <memory>
namespace {
std::unique_ptr<sys::LoggerSystem> logger_{nullptr};
}
namespace common::logger {

void init() { logger_ = std::make_unique<sys::LoggerSystem>(); }

auto getLogger() -> sys::LoggerSystem* {
    if (!logger_) {
        init();
    }
    return logger_.get();
}

}  // namespace common::logger