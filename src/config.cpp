#include "config.hpp"

#include <spdlog/spdlog.h>
#define YAML_CPP_API
#include <yaml-cpp/yaml.h>

#include <mutex>
namespace g {
template VulkanConfig Config::getConfig<VulkanConfig>() const;
template LogConfig Config::getConfig<LogConfig>() const;

namespace {
std::once_flag init_flag;
YAML::Node config;
auto parseVulkanConfig() -> VulkanConfig {
    return {.validationLayers = config["vulkan"]["validation_layers"].as<bool>()};
}
auto parseLogConfig() -> LogConfig {
    return {.level = config["log"]["level"].as<std::string>(),
            .pattern = config["log"]["pattern"].as<std::string>(),
            .console = config["log"]["console"]["enabled"].as<bool>(),
            .file = config["log"]["file"]["enabled"].as<bool>(),
            .file_path = config["log"]["file"]["path"].as<std::string>(),
            .append = config["log"]["file"]["append"].as<bool>()};
}
}  // namespace
Config::Config(std::string_view path) {
    std::call_once(init_flag, [&] { config = YAML::LoadFile(path.data()); });
}
Config::Config() {
    std::call_once(init_flag, [&] { throw std::runtime_error("Config file not set"); });
}

template <typename T>
auto Config::getConfig() const -> T {
    if constexpr (std::is_same<T, VulkanConfig>::value) {
        return parseVulkanConfig();
    }
    if constexpr (std::is_same<T, LogConfig>::value) {
        return parseLogConfig();
    }
}
}  // namespace g