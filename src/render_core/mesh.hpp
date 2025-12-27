#pragma once

#include <span>
#include <vector>
#include "common/class_traits.hpp"
#include "render_core/vertex.hpp"
namespace render {
class IMeshData {
    public:
        IMeshData() = default;
        virtual ~IMeshData() = default;
        CLASS_DEFAULT_COPYABLE(IMeshData);
        CLASS_DEFAULT_MOVEABLE(IMeshData);
        [[nodiscard]] virtual auto getMesh() const -> std::span<const float> = 0;
        [[nodiscard]] virtual auto getVertexCount() const -> std::size_t = 0;
        [[nodiscard]] virtual auto getIndices() const -> std::span<const std::byte> = 0;
        [[nodiscard]] virtual auto getIndicesSize() const -> std::uint64_t = 0;
        [[nodiscard]] virtual auto getVertexAttribute() const
            -> std::vector<render::VertexAttribute> = 0;
        [[nodiscard]] virtual auto getVertexBinding() const
            -> std::vector<render::VertexBinding> = 0;
};
}  // namespace render