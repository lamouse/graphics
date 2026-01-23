#pragma once
#include "render_core/types.hpp"
#include <variant>
#include <boost/container/small_vector.hpp>

namespace render {
constexpr size_t MAX_DRAW_SHADER_STAGE = 5;

	struct DrawIndexCommand
	{
		//see shader_tools/stage.h for stage definition
		std::array<ShaderHash, MAX_DRAW_SHADER_STAGE> shaders{};
		MeshId mesh;
	};

	struct DrawInstanceCommand{
		//see shader_tools/stage.h for stage definition
		std::array<ShaderHash, MAX_DRAW_SHADER_STAGE> shaders{};
		MeshId mesh;
		uint32_t instanceCount{};
	};

	struct DrawCountCommand
	{
		//see shader_tools/stage.h for stage definition
		std::array<ShaderHash, MAX_DRAW_SHADER_STAGE> shaders{};
		MeshId mesh;
	};

	using DrawCommand = std::variant<DrawIndexCommand, DrawInstanceCommand, DrawCountCommand>;

	struct CommandList final{


		boost::container::small_vector<DrawCommand, 8> commands;
	};
}