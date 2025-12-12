#include "logger_system.hpp"
#include <spdlog/pattern_formatter.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/async.h>
#include "utils/log_util.hpp"
#include "common/settings.hpp"
#include <memory>
#include <filesystem>

namespace sys {

LoggerSystem::LoggerSystem()
    : log_level_(utils::get_log_level(settings::values.log_level.GetValue())) {
    const char* log_patten = "%^[%Y-%m-%d %H:%M:%S.%e] [%l] [thread %t] (%s:%# %!): %v%$";
    const char* file_patten = "[%Y-%m-%d %H:%M:%S.%e] [%l] : %v";
    // 初始化线程池
    std::size_t queue_size = 1024;  // 队列大小
    std::size_t thread_count = 1;   // 线程数量
    spdlog::init_thread_pool(queue_size, thread_count);
    // 创建多接收器日志器
    std::vector<spdlog::sink_ptr> sinks;
    if (settings::values.log_console.GetValue()) {
        // 创建控制台接收器
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(log_level_);
        console_sink->set_pattern(log_patten);
        sinks.push_back(console_sink);
    }
    if (settings::values.log_file.GetValue()) {
        auto log_path = std::filesystem::path("logs/graphics.log");
        // 创建文件接收器
        auto file_sink =
            std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_path.string(), true);
        file_sink->set_level(log_level_);
        file_sink->set_pattern(file_patten);
        sinks.push_back(file_sink);
    }
    imgui_sink = std::make_shared<ImGuiLogSink_mt>();
    sinks.push_back(imgui_sink);
    logger_ = std::make_shared<spdlog::async_logger>("multi_sink", sinks.begin(), sinks.end(),
                                                     spdlog::thread_pool(),
                                                     spdlog::async_overflow_policy::block);

    // 设置默认日志器
    spdlog::set_default_logger(logger_);
    // 设置日志级别和格式
    spdlog::set_pattern(log_patten);
    spdlog::set_level(log_level_);

    // 刷新日志器
    spdlog::flush_on(log_level_);
}

void LoggerSystem::drawUi(bool& show) {
    if (show) {
        imgui_sink->draw("Console", show);
    }
    const auto level = utils::get_log_level(settings::values.log_level.GetValue());
    if (log_level_ != level) {
        log_level_ = level;
        logger_->set_level(log_level_);
        // 刷新日志器
        spdlog::flush_on(log_level_);
    }
}
LoggerSystem::~LoggerSystem() {spdlog::shutdown();}
}  // namespace sys