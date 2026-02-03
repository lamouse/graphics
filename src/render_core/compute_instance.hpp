#pragma once
#include <span>
#include "render_core/types.hpp"
namespace render {
class IComputeInstance {
    public:
        [[nodiscard]] virtual auto getUBOData() const
            -> std::vector<std::span<const std::byte>> = 0;
        virtual ~IComputeInstance() = default;
        IComputeInstance() = default;
        [[nodiscard]] auto getMeshIds() const -> std::span<const MeshId> { return mesh_ids; }
        [[nodiscard]] auto getShaderHash() const { return compute_shader_hash; }
        [[nodiscard]] auto getWorkgroupSize() const -> const std::array<u32, 3>& {
            return workgroup_size;
        }
        IComputeInstance(const IComputeInstance&) = default;
        IComputeInstance(IComputeInstance&&)noexcept = default;
        auto operator=(const IComputeInstance&) -> IComputeInstance& = default;
        auto operator=(IComputeInstance&&) noexcept -> IComputeInstance& = default;
    protected:
        std::span<MeshId> mesh_ids;
        std::span<TextureId> texture_ids;
        std::uint64_t compute_shader_hash{};
        std::array<u32, 3> workgroup_size{};
};
}  // namespace render