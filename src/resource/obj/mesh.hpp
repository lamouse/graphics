#pragma once

#include <span>
#include <vector>
#include "common/common_funcs.hpp"
#include "render_core/vertex.hpp"
namespace graphics {
class IMeshData {
    public:
        IMeshData() = default;
        virtual ~IMeshData() = default;
        CLASS_DEFAULT_COPYABLE(IMeshData);
        CLASS_DEFAULT_MOVEABLE(IMeshData);
        [[nodiscard]] virtual auto getMesh() const -> std::span<const float> = 0;
        [[nodiscard]] virtual auto getIndices() const -> std::span<const std::byte> = 0;
        [[nodiscard]] virtual auto getIndicesSize() const -> std::uint64_t = 0;
        [[nodiscard]] virtual auto getVertexAttribute() const -> std::vector<render::VertexAttribute> = 0;
        [[nodiscard]] virtual auto getVertexBinding() const -> std::vector<render::VertexBinding> = 0;
        // 判断是否使用 32 位索引
        [[nodiscard]] virtual auto uses32BitIndices() const -> bool = 0;
};
}  // namespace graphics