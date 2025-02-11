#pragma once
#include "common_settings.hpp"
#include "settings_enums.hpp"
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
        bool renderer_force_max_clock = false;
        bool use_pipeline_cache = true;
        enums::VSyncMode vSyncMode = enums::VSyncMode::Mailbox;
        enums::VramUsageMode v_ram_usage_mode = settings::enums::VramUsageMode::Conservative;
        static auto get() { return RenderVulkan{}; }
};

class Graphics : public common::settings::BaseSetting<Graphics> {
        friend common::settings::BaseSetting<Graphics>;
        auto getImpl() -> Graphics { return *this; }

    public:
        int fsr_sharpening_slider = 50;
        enums::AstcRecompression astc_recompression;
        enums::AstcDecodeMode astc_decodeMode = enums::AstcDecodeMode::Gpu;
        enums::ScalingFilter scaling_filter = enums::ScalingFilter::NearestNeighbor;
        bool use_asynchronous_shaders = true;
        static auto get() { return Graphics{}; }
};

struct ResolutionScalingInfo {
        uint32_t up_scale{1};
        uint32_t down_shift{0};
        float up_factor{1.0f};
        float down_factor{1.0f};
        bool active{};
        bool downscale{};

        std::int32_t ScaleUp(std::int32_t value) const {
            if (value == 0) {
                return 0;
            }
            return std::max((value * static_cast<std::int32_t>(up_scale)) >>
                                static_cast<std::int32_t>(down_shift),
                            1);
        }

        uint32_t ScaleUp(uint32_t value) const {
            if (value == 0U) {
                return 0U;
            }
            return std::max((value * up_scale) >> down_shift, 1U);
        }
        static auto get() { return ResolutionScalingInfo{}; }
};
}  // namespace settings