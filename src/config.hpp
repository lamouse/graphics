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
        /**
         * @brief Construct a new Config object
         *
         * @param path
         */
        explicit Config(std::string_view path) { config = YAML::LoadFile(path.data()); }

        /**
         * @brief Get the Config object
         * @tparam T
         * @param self
         * @return decltype(auto)
         */
        template <typename T>
        auto getConfig(this auto&& self) -> decltype(auto) {
            return T::read_config(self.config[T::node_name()]);
        }
};
}  // namespace g
#endif