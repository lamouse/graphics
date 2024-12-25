#include "config.hpp"

#include <spdlog/spdlog.h>
#include <yaml-cpp/yaml.h>
namespace g {
Config::Config(std::string_view path) {
    YAML::Node config = YAML::LoadFile(path.data());
    spdlog::info("vulkan visible: {}", config["vulkan"]["validation_layers"].as<bool>());
}
}  // namespace g