#include "fixed_pipeline_state.h"
#include "common/cityhash.h"
namespace render::vulkan {
auto FixedPipelineState::Hash() const noexcept -> size_t {
    const u64 hash = common::CityHash64(reinterpret_cast<const char*>(this), Size());
    return static_cast<size_t>(hash);
}

auto FixedPipelineState::operator==(const FixedPipelineState& rhs) const noexcept -> bool {
    return std::memcmp(this, &rhs, Size()) == 0;
}
}  // namespace render::vulkan