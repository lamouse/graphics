#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include "../config.hpp"
#include "config/log.h"
#include "g_app.hpp"
#include "utils/log_util.hpp"

using namespace std;
using namespace g;
void init(const Config& config);
auto main(int /*argc*/, char** /*argv*/) -> int {
    try {
        Config config("config/config.yaml");
        init(config);
        g::App app(config);
        app.run();
    } catch (const ::std::exception& e) {
        ::spdlog::error(e.what());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

void init(const Config& config) {
    // ::spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] %^[%l]%$ [thread %t] (%s:%# %!): %v");
    auto logConfig = config.getConfig<config::log::Log>();
    auto level = utils::get_log_level_from_string(logConfig.level);

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
    auto logger = std::make_shared<spdlog::logger>("multi_sink", sinks.begin(), sinks.end());

    // 设置默认日志器
    spdlog::set_default_logger(logger);
    // 设置日志级别和格式
    spdlog::set_pattern(logConfig.pattern);
    spdlog::set_level(level);

    // 刷新日志器
    spdlog::flush_on(level);
}
