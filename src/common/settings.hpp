#pragma once
#include "settings_enums.hpp"
#include "common/settings_setting.hpp"
#undef max
#undef min
namespace settings {

auto TranslateCategory(Category category) -> const char*;

#define SETTING(TYPE, RANGED) extern template class Setting<TYPE, RANGED>

SETTING(enums::VSyncMode, true);
SETTING(enums::LogLevel, true);
SETTING(bool, false);
SETTING(int, true);
#undef SETTING

struct Values {
        Linkage linkage;
        SwitchableSetting<enums::VSyncMode, true> vsync_mode{linkage,
                                                             enums::VSyncMode::Fifo,
                                                             "use_vsync",
                                                             Category::render,
                                                             Specialization::RuntimeList,
                                                             true,
                                                             true};
        SwitchableSetting<enums::LogLevel, true> log_level{
            linkage, enums::LogLevel::debug, "", Category::log, Specialization::RuntimeList, true,
            true};
        Setting<enums::VramUsageMode, false> v_ram_usage_mode{
            linkage, enums::VramUsageMode::Conservative, "v_ram_usage_mode", Category::render};
        Setting<bool, false> use_present_thread{linkage, false, "use_present_thread",
                                                Category::render};
        Setting<bool, false> render_debug{linkage, true, "render_debug", Category::render};
        Setting<bool, false> renderer_shader_feedback{linkage, false, "renderer_shader_feedback",
                                                      Category::render};
        Setting<bool, false> enable_compute_pipelines{linkage, true, "enable_compute_pipelines",
                                                      Category::render};
        Setting<bool, false> use_pipeline_cache{linkage, true, "use_pipeline_cache",
                                                Category::render};
        Setting<bool, false> use_asynchronous_shaders{linkage, true, "use_asynchronous_shaders",
                                                      Category::render};
        Setting<bool, false> use_dynamic_rendering{linkage, true, "use_dynamic_rendering",
                                                   Category::render};

        Setting<bool, false> log_console{linkage, true, "log_console", Category::log};
        Setting<bool, false> log_file{linkage, true, "file", Category::log};
        Setting<int, true> fsr_sharpening_slider{
            linkage, 53, 0, 100, "fsr_sharpening_slider", Category::core, Specialization::Scalar,
            true};
        Setting<enums::ScalingFilter, true> scaling_filter{
            linkage,        enums::ScalingFilter::Fsr,   "scaling_filter",
            Category::core, Specialization::RuntimeList, true};

        Setting<enums::AspectRatio, true> aspect_ratio{linkage, enums::AspectRatio::R16_9,
                                                       "aspect_ratio", Category::core,
                                                       Specialization::List};

        Setting<enums::AstcRecompression, true> astc_recompression{
            linkage, enums::AstcRecompression::Uncompressed, "astc_recompression", Category::core,
            Specialization::List};

        Setting<enums::AstcDecodeMode, true> astc_decodeMode{linkage, enums::AstcDecodeMode::Gpu,
                                                             "astc_recompression", Category::core,
                                                             Specialization::List};
};

extern Values values;  // NOLINT
}  // namespace settings