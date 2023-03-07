#include "g_shader.hpp"
#include "g_device.hpp"
#include <fstream>
#include <stdexcept>
#include <iostream>
namespace g{

std::unique_ptr<Shader> Shader::instance = nullptr;

Shader::Shader(const ::std::string& vertFilePath, const ::std::string& fragFilePath)
{
    createGraphicsShader(vertFilePath, fragFilePath);
}

void Shader::init(const ::std::string& vertFilePath, const ::std::string& fragFilePath)
{
    instance.reset(new Shader(vertFilePath, fragFilePath));
}

void Shader::quit()
{
    instance.reset();
}

Shader::~Shader()
{
     Device::getInstance().getVKDevice().destroyShaderModule(vertexModule);
     Device::getInstance().getVKDevice().destroyShaderModule(fragmentModule);
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
    ::std::cout << "Vertex Size " << vertCode.size() << "\n";
    ::std::cout << "Frag Size " << fragCode.size() << "\n";
    ::vk::ShaderModuleCreateInfo createInfo;
    createInfo.setCodeSize(vertCode.size());
    createInfo.setPCode((uint32_t*)vertCode.data());
    vertexModule = Device::getInstance().getVKDevice().createShaderModule(createInfo);

    createInfo.setCodeSize(fragCode.size());
    createInfo.setPCode((uint32_t*)fragCode.data());
    fragmentModule = Device::getInstance().getVKDevice().createShaderModule(createInfo);

}

::std::vector<::vk::PipelineShaderStageCreateInfo> Shader::getShaderStage()
{
    shaderStages[0].setStage(::vk::ShaderStageFlagBits::eVertex)
                .setModule(vertexModule)
                .setPName("main");

    shaderStages[1].setStage(::vk::ShaderStageFlagBits::eVertex)
                .setModule(fragmentModule)
                .setPName("main");

}


}