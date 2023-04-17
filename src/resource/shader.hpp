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
        ::vk::ShaderModule getVertex(){return vertexModule; }
        ::vk::ShaderModule getFragmentModule(){return fragmentModule; }
        ::std::vector<::vk::PipelineShaderStageCreateInfo> getShaderStages() override;
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
        ::vk::ShaderModule getShaderModule(){return computeModule; }
        ::vk::PipelineShaderStageCreateInfo getShaderStages() override;
        ~ComputeShader();
        ComputeShader(::vk::Device& device, const ::std::string& filePath);
    };

}

#endif