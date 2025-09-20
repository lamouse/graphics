#include "fixed_pipeline_state.h"
#include <farmhash.h>
namespace render {
auto FixedPipelineState::Hash() const noexcept -> size_t {
    const u64 hash = NAMESPACE_FOR_HASH_FUNCTIONS::Fingerprint64(reinterpret_cast<const char*>(this), Size());
    return static_cast<size_t>(hash);
}

auto FixedPipelineState::operator==(const FixedPipelineState& rhs) const noexcept -> bool {
    return std::memcmp(this, &rhs, Size()) == 0;
}
}  // namespace render