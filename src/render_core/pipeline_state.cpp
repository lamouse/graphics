#include "pipeline_state.h"
#include <xxhash.h>
#include <nlohmann/json.hpp>

namespace nlohmann {

// ===== 1. ComparisonOp =====
template<>
struct adl_serializer<render::ComparisonOp> {
    static void to_json(json& j, const render::ComparisonOp& op) {
        j = static_cast<std::underlying_type_t<render::ComparisonOp>>(op);
    }
    static void from_json(const json& j, render::ComparisonOp& op) {
        op = static_cast<render::ComparisonOp>(j.get<std::underlying_type_t<render::ComparisonOp>>());
    }
};

// ===== 2. StencilOp::Op =====
template<>
struct adl_serializer<render::StencilOp::Op> {
    static void to_json(json& j, const render::StencilOp::Op& op) {
        j = static_cast<std::underlying_type_t<render::StencilOp::Op>>(op);
    }
    static void from_json(const json& j, render::StencilOp::Op& op) {
        op = static_cast<render::StencilOp::Op>(j.get<std::underlying_type_t<render::StencilOp::Op>>());
    }
};

// ===== 3. CullFace =====
template<>
struct adl_serializer<render::CullFace> {
    static void to_json(json& j, const render::CullFace& face) {
        j = static_cast<std::underlying_type_t<render::CullFace>>(face);
    }
    static void from_json(const json& j, render::CullFace& face) {
        face = static_cast<render::CullFace>(j.get<std::underlying_type_t<render::CullFace>>());
    }
};

// ===== 4. StencilOp =====
template<>
struct adl_serializer<render::StencilOp> {
    static void to_json(json& j, const render::StencilOp& s) {
        j = json{
            {"fail", s.fail},
            {"pass", s.pass},
            {"depthFail", s.depthFail},
            {"compare", s.compare}
        };
    }
    static void from_json(const json& j, render::StencilOp& s) {
        j.at("fail").get_to(s.fail);
        j.at("pass").get_to(s.pass);
        j.at("depthFail").get_to(s.depthFail);
        j.at("compare").get_to(s.compare);
    }
};

// ===== 5. ViewPort =====
template<>
struct adl_serializer<render::DynamicPipelineState::ViewPort> {
    static void to_json(json& j, const render::DynamicPipelineState::ViewPort& v) {
        j = json{
            {"x", v.x},
            {"y", v.y},
            {"width", v.width},
            {"height", v.height},
            {"minDepth", v.minDepth},
            {"maxDepth", v.maxDepth}
        };
    }
    static void from_json(const json& j, render::DynamicPipelineState::ViewPort& v) {
        j.at("x").get_to(v.x);
        j.at("y").get_to(v.y);
        j.at("width").get_to(v.width);
        j.at("height").get_to(v.height);
        v.minDepth = j.value("minDepth", 0.0f);
        v.maxDepth = j.value("maxDepth", 1.0f);
    }
};

// ===== 6. Scissors =====
template<>
struct adl_serializer<render::DynamicPipelineState::Scissors> {
    static void to_json(json& j, const render::DynamicPipelineState::Scissors& s) {
        j = json{
            {"x", s.x},
            {"y", s.y},
            {"width", s.width},
            {"height", s.height}
        };
    }
    static void from_json(const json& j, render::DynamicPipelineState::Scissors& s) {
        j.at("x").get_to(s.x);
        j.at("y").get_to(s.y);
        j.at("width").get_to(s.width);
        j.at("height").get_to(s.height);
    }
};

// ===== 7. BlendColor =====
template<>
struct adl_serializer<render::DynamicPipelineState::BlendColor> {
    static void to_json(json& j, const render::DynamicPipelineState::BlendColor& c) {
        j = json{{"r", c.r}, {"g", c.g}, {"b", c.b}, {"a", c.a}};
    }
    static void from_json(const json& j, render::DynamicPipelineState::BlendColor& c) {
        c.r = j.value("r", 0.0f);
        c.g = j.value("g", 0.0f);
        c.b = j.value("b", 0.0f);
        c.a = j.value("a", 0.0f);
    }
};

// ===== 8. StencilProperties =====
template<>
struct adl_serializer<render::DynamicPipelineState::StencilProperties> {
    static void to_json(json& j, const render::DynamicPipelineState::StencilProperties& p) {
        j = json{
            {"ref", p.ref},
            {"write_mask", p.write_mask},
            {"compare_mask", p.compare_mask}
        };
    }
    static void from_json(const json& j, render::DynamicPipelineState::StencilProperties& p) {
        p.ref = j.value("ref", 0u);
        p.write_mask = j.value("write_mask", 255u);
        p.compare_mask = j.value("compare_mask", 255u);
    }
};

// ===== 9. DynamicPipelineState =====
template<>
struct adl_serializer<render::DynamicPipelineState> {
    static void to_json(json& j, const render::DynamicPipelineState& state) {
        j = json{
            // 位域 flags → 单独字段（更清晰，避免 union 序列化问题）
            {"colorBlendEnable", state.colorBlendEnable},
            {"stencilTestEnable", state.stencilTestEnable},
            {"depthClampEnable", state.depthClampEnable},
            {"depthWriteEnable", state.depthWriteEnable},
            {"depthTestEnable", state.depthTestEnable},
            {"depthBoundsTestEnable", state.depthBoundsTestEnable},
            {"cullMode", state.cullMode},
            {"depthBiasEnable", state.depthBiasEnable},
            {"rasterizerDiscardEnable", state.rasterizerDiscardEnable},
            {"primitiveRestartEnable", state.primitiveRestartEnable},
            {"stencil_two_side_enable", state.stencil_two_side_enable},

            // 嵌套结构
            {"viewport", state.viewport},
            {"scissors", state.scissors},
            {"blendColor", state.blendColor},
            {"frontStencilOp", state.frontStencilOp},
            {"backStencilOp", state.backStencilOp},
            {"stencilFrontProperties", state.stencilFrontProperties},
            {"stencilBackProperties", state.stencilBackProperties},
            {"depthComparison", state.depthComparison},
            {"cullFace", state.cullFace}
        };
    }

    static void from_json(const json& j, render::DynamicPipelineState& state) {
        // 位域字段（使用 value 提供默认值，兼容旧配置）
        state.colorBlendEnable = j.value("colorBlendEnable", 1u);
        state.stencilTestEnable = j.value("stencilTestEnable", 1u);
        state.depthClampEnable = j.value("depthClampEnable", 1u);
        state.depthWriteEnable = j.value("depthWriteEnable", 1u);
        state.depthTestEnable = j.value("depthTestEnable", 1u);
        state.depthBoundsTestEnable = j.value("depthBoundsTestEnable", 1u);
        state.cullMode = j.value("cullMode", 0u);
        state.depthBiasEnable = j.value("depthBiasEnable", 1u);
        state.rasterizerDiscardEnable = j.value("rasterizerDiscardEnable", 0u);
        state.primitiveRestartEnable = j.value("primitiveRestartEnable", 0u);
        state.stencil_two_side_enable = j.value("stencil_two_side_enable", 0u);

        // 嵌套对象
        if (j.contains("viewport")) j.at("viewport").get_to(state.viewport);
        if (j.contains("scissors")) j.at("scissors").get_to(state.scissors);
        if (j.contains("blendColor")) j.at("blendColor").get_to(state.blendColor);
        if (j.contains("frontStencilOp")) j.at("frontStencilOp").get_to(state.frontStencilOp);
        if (j.contains("backStencilOp")) j.at("backStencilOp").get_to(state.backStencilOp);
        if (j.contains("stencilFrontProperties")) j.at("stencilFrontProperties").get_to(state.stencilFrontProperties);
        if (j.contains("stencilBackProperties")) j.at("stencilBackProperties").get_to(state.stencilBackProperties);
        if (j.contains("depthComparison")) j.at("depthComparison").get_to(state.depthComparison);
        if (j.contains("cullFace")) j.at("cullFace").get_to(state.cullFace);
    }
};

} // namespace nlohmann
namespace render {
auto FixedPipelineState::Hash() const noexcept -> size_t {
    const u64 hash = XXH64(reinterpret_cast<const char*>(this), Size(), 0);
    return static_cast<size_t>(hash);
}

auto FixedPipelineState::operator==(const FixedPipelineState& rhs) const noexcept -> bool {
    return std::memcmp(this, &rhs, Size()) == 0;
}

void FixedPipelineState::Refresh(DynamicFeatures& features) {
    raw1 = 0;
    extended_dynamic_state.Assign(features.has_extended_dynamic_state ? 1 : 0);
    extended_dynamic_state_2.Assign(features.has_extended_dynamic_state_2 ? 1 : 0);
    extended_dynamic_state_2_extra.Assign(features.has_extended_dynamic_state_2_extra ? 1 : 0);
    extended_dynamic_state_3_blend.Assign(features.has_extended_dynamic_state_3_blend ? 1 : 0);
    extended_dynamic_state_3_enables.Assign(features.has_extended_dynamic_state_3_enables ? 1 : 0);
    dynamic_vertex_input.Assign(features.has_dynamic_vertex_input ? 1 : 0);
}

[[nodiscard]] auto DynamicPipelineState::to_json_string() const -> std::string{
    nlohmann::json j = *this;
    return j.dump(4);
}

auto DynamicPipelineState::from_json_string(const std::string& json_str) -> DynamicPipelineState{
    nlohmann::json j = nlohmann::json::parse(json_str);
    render::DynamicPipelineState state = j.get<render::DynamicPipelineState>();
    return state;
}
}  // namespace render


