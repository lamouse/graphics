#pragma once
#include "common_settings.hpp"
#include "settings_enums.hpp"
#include "common/settings_setting.hpp"
#undef max
#undef min
namespace settings {

class RenderVulkan : public common::settings::BaseSetting<RenderVulkan> {
        friend common::settings::BaseSetting<RenderVulkan>;
        auto getImpl() -> RenderVulkan { return *this; }

    public:
        // 这里可以用来统计渲染器的性能 主要在调试的时候使用 相关文件是
        // src\render_core\render_vulkan\pipeline_statistics.hpp
        bool renderer_shader_feedback = false;
        bool enable_compute_pipelines = true;
        bool use_present_thread = false;
        bool async_presentation = false;
        bool render_debug = true;
        bool use_pipeline_cache = true;
        enums::VramUsageMode v_ram_usage_mode = settings::enums::VramUsageMode::Conservative;
        static auto get() { return instance_; }

    private:
        static RenderVulkan instance_;
};

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
    SwitchableSetting<enums::LogLevel, true> log_level{linkage,
                                                       enums::LogLevel::info,
                                                       "",
                                                       Category::log,
                                                       Specialization::RuntimeList,
                                                       true,
                                                       true};
    Setting<bool, false> log_console{linkage, true, "log_console", Category::log};
    Setting<bool, false> log_file{linkage, true, "file", Category::log};
};

extern Values values;
} // namespace settings