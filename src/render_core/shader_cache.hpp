#pragma once
#include "common/common_types.hpp"
#include <unordered_map>
#include <memory>
#include <span>
#include <vector>
#include <limits>
namespace render {
enum class ShaderType : u8 {
    Vertex = 0,
    Fragment = 1,
    Compute = 2,

    NONE = std::numeric_limits<u8>::max()
};
struct ShaderInfo {
        u64 unique_hash{};
        ShaderType type{};
        bool loaded{false};
};

class ShaderCache {
        static constexpr u8 NUM_PROGRAMS = 3;

    public:
        virtual ~ShaderCache() = default;  // 如果你打算多态删除
        ShaderCache() = default;
        ShaderCache(const ShaderCache&) = delete;
        ShaderCache(ShaderCache&&) noexcept = default;
        auto operator=(const ShaderCache&) -> ShaderCache& = delete;
        auto operator=(ShaderCache&&) noexcept -> ShaderCache& = default;

        // 返回计算后的hash
        auto addShader(std::span<const u32> data, ShaderType type) -> u64;
        void setCurrentShader(u64 vertexHash, u64 fragmentHas);
        void setCurrentShader(u64 computeHash);

    protected:
        std::array<const ShaderInfo*, NUM_PROGRAMS> shader_infos{};
        void mark_loaded(const std::array<const ShaderInfo*, NUM_PROGRAMS>& infos);
        auto getShaderData(u64 hash) -> std::span<const u32>;

    private:
        std::unordered_map<u64, std::unique_ptr<ShaderInfo>> storage;
        std::unordered_map<u64, std::vector<u32>> load_data;
};

}  // namespace render