#pragma once
#include <memory>
#include <string>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace resource::shader {
class GraphicsShader {
    private:
        void createGraphicsShader(const ::std::string& vertFilePath, const ::std::string& fragFilePath);
        ::vk::ShaderModule vertexModule;
        ::vk::ShaderModule fragmentModule;
        ::vk::Device& device_;

    public:
        auto getVertex() -> ::vk::ShaderModule { return vertexModule; }
        auto getFragmentModule() -> ::vk::ShaderModule { return fragmentModule; }
        auto getShaderStages() -> ::std::vector<::vk::PipelineShaderStageCreateInfo>;
        ~GraphicsShader();
        GraphicsShader(const ::std::string& vertFilePath, const ::std::string& fragFilePath, ::vk::Device& device);
};

class ComputeShader {
    private:
        void createComputeShader(const ::std::string& filePath);
        ::vk::ShaderModule computeModule;
        ::vk::Device& device_;

    public:
        auto getShaderModule() -> ::vk::ShaderModule { return computeModule; }
        auto getShaderStages() -> ::vk::PipelineShaderStageCreateInfo;
        ~ComputeShader();
        ComputeShader(::vk::Device& device, const ::std::string& filePath);
};

}  // namespace resource::shader
