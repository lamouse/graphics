#pragma once
#ifndef CONFIG_HPP
#define CONFIG_HPP
#define YAML_CPP_API
#include <yaml-cpp/yaml.h>

#include <string_view>
namespace g {
class Config {
    private:
        YAML::Node config;

    public:
        explicit Config(std::string_view path) { config = YAML::LoadFile(path.data()); }
        template <typename T>
        auto getConfig() const -> T {
            return T::read_config(config[T::node_name()]);
        }
};
}  // namespace g
#endif