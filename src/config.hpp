#pragma once
#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <string_view>
namespace g {
struct VulkanConfig {
        bool validationLayers;
};
struct LogConfig {
        std::string level;
        std::string pattern;
        bool console;
        bool file;
        std::string file_path;
        bool append;
};
class Config {
    public:
        explicit Config(std::string_view path);
        Config();
        template <typename T>
        auto getConfig() const -> T;
};
}  // namespace g
#endif