#include "shader.hpp"
#include <fstream>
#include <stdexcept>
#include <iostream>
namespace resource::shader{


::std::vector<char> readShaderFile(const std::string& filePath){
    ::std::ifstream file{filePath, ::std::ios::ate | ::std::ios::binary};
    if(!file.is_open()){
        throw ::std::runtime_error("faild to open file: " + filePath);
    }
    size_t fileSize = static_cast<size_t>(file.tellg());
    ::std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    return buffer;
}


GraphicsShader::GraphicsShader(const ::std::string& vertFilePath, const ::std::string& fragFilePath, ::vk::Device& device):device_(device)
{
    createGraphicsShader(vertFilePath, fragFilePath);
}

GraphicsShader::~GraphicsShader()
{
    device_.destroyShaderModule(vertexModule);
    device_.destroyShaderModule(fragmentModule);
}



void GraphicsShader::createGraphicsShader(const ::std::string& vertFilePath, const ::std::string& fragFilePath){
    auto vertCode = readShaderFile(vertFilePath);
    auto fragCode = readShaderFile(fragFilePath);
    ::vk::ShaderModuleCreateInfo createInfo;
    createInfo.setCodeSize(vertCode.size());
    createInfo.setPCode((uint32_t*)vertCode.data());
    vertexModule = device_.createShaderModule(createInfo);

    createInfo.setCodeSize(fragCode.size());
    createInfo.setPCode((uint32_t*)fragCode.data());
    fragmentModule = device_.createShaderModule(createInfo);

}

::std::vector<::vk::PipelineShaderStageCreateInfo> GraphicsShader::getShaderStages()
{
    ::std::vector<::vk::PipelineShaderStageCreateInfo> shaderStages;
    shaderStages.resize(2);
    shaderStages[0].setStage(::vk::ShaderStageFlagBits::eVertex)
                .setModule(vertexModule)
                .setPName("main");

    shaderStages[1].setStage(::vk::ShaderStageFlagBits::eFragment)
                .setModule(fragmentModule)
                .setPName("main");
    return shaderStages;
}

ComputeShader::ComputeShader(::vk::Device& device, const ::std::string& filePath):device_(device)
{
    createComputeShader(filePath);
}

void ComputeShader::createComputeShader(const ::std::string& filePath)
{
    auto computeCode = readShaderFile(filePath);
    ::vk::ShaderModuleCreateInfo createInfo;
    createInfo.setCodeSize(computeCode.size());
    createInfo.setPCode((uint32_t*)computeCode.data());
    computeModule = device_.createShaderModule(createInfo);
}

ComputeShader::~ComputeShader()
{
    device_.destroyShaderModule(computeModule);
}
::vk::PipelineShaderStageCreateInfo ComputeShader::getShaderStages()
{
    return {{}, ::vk::ShaderStageFlagBits::eCompute, computeModule, "main",};
}

}