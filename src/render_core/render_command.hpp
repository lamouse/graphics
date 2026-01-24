#pragma once
#include "render_core/types.hpp"
#include "render_core/pipeline_state.h"
#include <boost/container/small_vector.hpp>
#include <variant>
#include <span>

namespace render {
constexpr size_t MAX_DRAW_SHADER_STAGE = 5;

struct DrawIndexCommand {
        // see shader_tools/stage.h for stage definition
        std::array<ShaderHash, MAX_DRAW_SHADER_STAGE> shaders{};
        PrimitiveTopology topology{};
        DynamicPipelineState pipelineState;
        std::span<std::span<const std::byte>> ubos;
        std::span<const std::byte> push_constants;
        std::span<const render::TextureId> textures;
        std::uint32_t index_offset{};
        std::uint32_t index_count{};
        std::uint32_t instance_count{1};
        MeshId mesh;
};

struct DrawInstanceCommand {
        // see shader_tools/stage.h for stage definition
        std::array<ShaderHash, MAX_DRAW_SHADER_STAGE> shaders{};
        MeshId mesh;
        PrimitiveTopology topology{};
        std::span<std::span<const std::byte>> ubos;
        std::span<const std::byte> push_constants;
        std::span<const render::TextureId> textures;
        DynamicPipelineState pipelineState;
        uint32_t instanceCount{};
};

struct DrawCountCommand {
        // see shader_tools/stage.h for stage definition
        std::array<ShaderHash, MAX_DRAW_SHADER_STAGE> shaders{};
        PrimitiveTopology topology{};
        DynamicPipelineState pipelineState;
        std::span<std::span<const std::byte>> ubos;
        std::span<const std::byte> push_constants;
        std::span<const render::TextureId> textures;
        MeshId mesh;
};

using DrawCommand = std::variant<DrawIndexCommand, DrawInstanceCommand, DrawCountCommand>;

struct CommandList final {
        boost::container::small_vector<DrawCommand, 512> commands;
};
}  // namespace render