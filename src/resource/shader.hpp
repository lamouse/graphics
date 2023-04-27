#ifndef G_PIPLINE_HPP
#define G_PIPLINE_HPP
#include "shader_stage.hpp"
#include <vulkan/vulkan.hpp>
#include <string>
#include <vector>
#include <memory>

namespace resource::shader{
    class GraphicsShader : public ShaderStage<::std::vector<::vk::PipelineShaderStageCreateInfo>>
    {
    private:
        void createGraphicsShader(const ::std::string& vertFilePath, const ::std::string& fragFilePath);
        ::vk::ShaderModule vertexModule;
        ::vk::ShaderModule fragmentModule;
        ::vk::Device& device_;
    public:
        auto getVertex() -> ::vk::ShaderModule{return vertexModule; }
        auto getFragmentModule() -> ::vk::ShaderModule{return fragmentModule; }
        auto getShaderStages() -> ::std::vector<::vk::PipelineShaderStageCreateInfo> override;
        ~GraphicsShader();
        GraphicsShader(const ::std::string& vertFilePath, const ::std::string& fragFilePath, ::vk::Device& device);
    };
    

    class ComputeShader : public ShaderStage<::vk::PipelineShaderStageCreateInfo>
    {
    private:
        
        void createComputeShader(const ::std::string& filePath);
        ::vk::ShaderModule computeModule;
        ::vk::Device& device_;
    public:
        auto getShaderModule() -> ::vk::ShaderModule{return computeModule; }
        auto getShaderStages() -> ::vk::PipelineShaderStageCreateInfo override;
        ~ComputeShader();
        ComputeShader(::vk::Device& device, const ::std::string& filePath);
    };

}

#endif