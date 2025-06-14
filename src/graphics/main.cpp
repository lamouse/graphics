#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#if defined(_WIN32)
#include <windows.h>
#endif
#include "../config.hpp"
#include "config/log.h"
#include "app.hpp"
#include "utils/log_util.hpp"

using namespace std;
using namespace g;
void init(const Config& config);
auto main(int /*argc*/, char** /*argv*/) -> int {
#if defined(_WIN32)
    SetConsoleOutputCP(65001);
#endif
    try {
        Config config("config/config.yaml");
        init(config);

        graphics::App app(config);
        app.run();

    } catch (const ::std::exception& e) {
        ::spdlog::error(e.what());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

void init(const Config& config) {
    auto logConfig = config.getConfig<config::log::Log>();
    auto level = utils::get_log_level_from_string(logConfig.level);
    // 初始化线程池
    std::size_t queue_size = 8192;  // 队列大小
    std::size_t thread_count = 1;   // 线程数量
    spdlog::init_thread_pool(queue_size, thread_count);
    // 创建多接收器日志器
    std::vector<spdlog::sink_ptr> sinks;
    if (logConfig.console.enabled) {
        // 创建控制台接收器
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(level);
        console_sink->set_pattern(logConfig.pattern);
        sinks.push_back(console_sink);
    }
    if (logConfig.file.enabled) {
        // 创建文件接收器
        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
            logConfig.file.path, !logConfig.file.append);
        sinks.push_back(file_sink);
    }
    auto logger = std::make_shared<spdlog::async_logger>("multi_sink", sinks.begin(), sinks.end(),
                                                         spdlog::thread_pool(),
                                                         spdlog::async_overflow_policy::block);

    // 设置默认日志器
    spdlog::set_default_logger(logger);
    // 设置日志级别和格式
    spdlog::set_pattern(logConfig.pattern);
    spdlog::set_level(level);

    // 刷新日志器
    spdlog::flush_on(level);
}
