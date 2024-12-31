#pragma once
#include "common_settings.hpp"
#include "settings_enums.hpp"
namespace settings {

class RenderVulkan : public common::settings::BaseSetting<RenderVulkan> {
        friend common::settings::BaseSetting<RenderVulkan>;
        auto getImpl() -> RenderVulkan { return *this; }

    public:
        bool renderer_shader_feedback = false;
        bool enable_compute_pipelines = true;
        enums::VSyncMode vSyncMode = enums::VSyncMode::Mailbox;
        enums::VramUsageMode v_ram_usage_mode = settings::enums::VramUsageMode::Conservative;
        static auto get() { return RenderVulkan{}; }
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
            return std::max((value * static_cast<std::int32_t>(up_scale)) >> static_cast<std::int32_t>(down_shift), 1);
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