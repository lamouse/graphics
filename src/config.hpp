#pragma once
#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string_view>
namespace g {

class Config {
    public:
        explicit Config(std::string_view path);
};
}  // namespace g
#endif