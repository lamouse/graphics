#include "settings.hpp"
#include "common/file.hpp"
#include <toml++/toml.h>
#include <spdlog/spdlog.h>
#include <fstream>

constexpr auto SETTING_FILE_NAME = "config.toml";
namespace settings {
Values values;  // NOLINT

#define SETTING(TYPE, RANGED) template class Setting<TYPE, RANGED>
SETTING(enums::VSyncMode, true);
SETTING(enums::LogLevel, true);
SETTING(bool, false);
SETTING(int, true);
#undef SETTING

auto TranslateCategory(Category category) -> const char* {
    switch (category) {
        case Category::core:
            return "Core";
        case Category::render:
            return "Render";
        case Category::log:
            return "Log";
        case Category::system:
            return "System";
        case Category::MAX_EUM:
            break;
    }
    return "Miscellaneous";
}

void save_settings() {
    toml::table tables;
    tables.insert("title", "Graphics Settings");
    tables.insert("version", "1.0.0");

    for (const auto& [category, setting] : settings::values.linkage.by_category) {
        toml::table setting_table;

        for (auto* s : setting) {
            if (s->Save()) {
                setting_table.insert(s->GetLabel(), s->ToString());
            }
        }
        tables.insert(settings::TranslateCategory(category), std::move(setting_table));
    }

    toml::table resolution_table;
    resolution_table.insert("width", values.resolution.width);
    resolution_table.insert("height", values.resolution.height);
    tables.insert("resolution", resolution_table);

    auto config_file = common::get_module_path(common::ModuleType::Config) /= SETTING_FILE_NAME;
    std::ofstream file(config_file);
    if (file.is_open()) {
        file << tables;  // toml::table 重载了 << 运算符
        file.close();
    }
    spdlog::info("setting file saved: {}", config_file.string());
}

void load_settings() {
    try {
        using namespace std::literals;
        auto config_file = common::get_module_path(common::ModuleType::Config) /= SETTING_FILE_NAME;
        auto toml_data = toml::parse_file(config_file.string());
        for (const auto& [category, setting] : settings::values.linkage.by_category) {
            for (auto* s : setting) {
                if (s->Save()) {
                    if (auto value = toml_data[settings::TranslateCategory(category)][s->GetLabel()]
                                         .value_or(""sv);
                        not value.empty()) {
                        s->LoadString(std::string(value));
                    }
                }
            }
        }
        values.resolution.width =
            toml_data["resolution"]["width"].value_or(values.resolution.width);
        values.resolution.height =
            toml_data["resolution"]["height"].value_or(values.resolution.height);
        spdlog::info("setting file loaded: {}", config_file.string());
    } catch (std::exception& e) {
        spdlog::info("failed to load settings: {}", e.what());
    }
}

}  // namespace settings