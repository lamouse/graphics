#pragma once
#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string_view>
namespace g {
struct VulkanConfig {
        bool validationLayers;
};
class Config {
    public:
        explicit Config(std::string_view path);
        Config();
        template <typename T>
        auto getConfig() -> T;
};
}  // namespace g
#endif