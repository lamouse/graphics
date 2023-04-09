#include "shader.hpp"
#include <fstream>
#include <stdexcept>
#include <iostream>
namespace resource::shader{


Shader::Shader(const ::std::string& vertFilePath, const ::std::string& fragFilePath, ::vk::Device& device):device(device)
{
    createGraphicsShader(vertFilePath, fragFilePath);
}

Shader::~Shader()
{
    device.destroyShaderModule(vertexModule);
    device.destroyShaderModule(fragmentModule);
}

::std::vector<char> Shader::readFile(const std::string& filePath){
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

void Shader::createGraphicsShader(const ::std::string& vertFilePath, const ::std::string& fragFilePath){
    auto vertCode = readFile(vertFilePath);
    auto fragCode = readFile(fragFilePath);
    ::vk::ShaderModuleCreateInfo createInfo;
    createInfo.setCodeSize(vertCode.size());
    createInfo.setPCode((uint32_t*)vertCode.data());
    vertexModule = device.createShaderModule(createInfo);

    createInfo.setCodeSize(fragCode.size());
    createInfo.setPCode((uint32_t*)fragCode.data());
    fragmentModule = device.createShaderModule(createInfo);

}

::std::vector<::vk::PipelineShaderStageCreateInfo> Shader::getShaderStage()
{
    shaderStages.resize(2);
    shaderStages[0].setStage(::vk::ShaderStageFlagBits::eVertex)
                .setModule(vertexModule)
                .setPName("main");

    shaderStages[1].setStage(::vk::ShaderStageFlagBits::eFragment)
                .setModule(fragmentModule)
                .setPName("main");
    return shaderStages;
}

}