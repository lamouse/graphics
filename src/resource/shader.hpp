#ifndef G_PIPLINE_HPP
#define G_PIPLINE_HPP
#include "shader_stage.hpp"
#include <vulkan/vulkan.hpp>
#include <string>
#include <vector>
#include <memory>

namespace resource::shader{
    class Shader : public ShaderStage<::vk::PipelineShaderStageCreateInfo>
    {
    private:
        static ::std::vector<char> readFile(const std::string& filePath);
        void createGraphicsShader(const ::std::string& vertFilePath, const ::std::string& fragFilePath);
        ::vk::ShaderModule vertexModule;
        ::vk::ShaderModule fragmentModule;
        ::std::vector<::vk::PipelineShaderStageCreateInfo> shaderStages;
        ::vk::Device& device;
    public:
        ::vk::ShaderModule getVertex(){return vertexModule; }
        ::vk::ShaderModule getFragmentModule(){return fragmentModule; }
        ::std::vector<::vk::PipelineShaderStageCreateInfo> getShaderStages() override;
        ~Shader();
        Shader(const ::std::string& vertFilePath, const ::std::string& fragFilePath, ::vk::Device& device);
    };
    
}

#endif