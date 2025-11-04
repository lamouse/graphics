#pragma once
#include "common_settings.hpp"
#include "settings_enums.hpp"
#include "common/settings_setting.hpp"
#undef max
#undef min
namespace settings {

class Graphics : public common::settings::BaseSetting<Graphics> {
        friend common::settings::BaseSetting<Graphics>;
        auto getImpl() -> Graphics { return *this; }

    public:
        int fsr_sharpening_slider = 50;
        enums::AstcRecompression astc_recompression;
        enums::AstcDecodeMode astc_decodeMode = enums::AstcDecodeMode::Gpu;
        enums::ScalingFilter scaling_filter = enums::ScalingFilter::Fsr;
        enums::AspectRatio aspect_ratio = enums::AspectRatio::R16_9;
        bool use_asynchronous_shaders = true;
        static auto get() { return Graphics{}; }
};

auto TranslateCategory(Category category) -> const char*;

#define SETTING(TYPE, RANGED) extern template class Setting<TYPE, RANGED>

SETTING(enums::VSyncMode, true);
SETTING(enums::LogLevel, true);
SETTING(bool, false);

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

        Setting<bool, false> log_console{linkage, true, "log_console", Category::log};
        Setting<bool, false> log_file{linkage, true, "file", Category::log};
};

extern Values values;
}  // namespace settings