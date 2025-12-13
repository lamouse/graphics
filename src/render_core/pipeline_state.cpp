#include "pipeline_state.h"
#include <xxhash.h>
namespace render {
auto FixedPipelineState::Hash() const noexcept -> size_t {
    const u64 hash =
        XXH64(reinterpret_cast<const char*>(this), Size(), 0);
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
}  // namespace render
