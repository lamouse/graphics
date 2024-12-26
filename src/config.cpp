#include "config.hpp"

#include <spdlog/spdlog.h>
#define YAML_CPP_API
#include <yaml-cpp/yaml.h>

#include <mutex>
namespace g {
template VulkanConfig Config::getConfig<VulkanConfig>();

namespace {
std::once_flag init_flag;
YAML::Node config;
auto parseVulkanConfig() -> VulkanConfig {
    return {.validationLayers = config["vulkan"]["validation_layers"].as<bool>()};
}
}  // namespace
Config::Config(std::string_view path) {
    std::call_once(init_flag, [&] { config = YAML::LoadFile(path.data()); });
}
Config::Config() {
    std::call_once(init_flag, [&] { throw std::runtime_error("Config file not set"); });
}

template <typename T>
auto Config::getConfig() -> T {
    if constexpr (std::is_same<T, VulkanConfig>::value) {
        return parseVulkanConfig();
    }
}
}  // namespace g