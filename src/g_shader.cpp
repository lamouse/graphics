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
    shaderStages.resize(2);
    shaderStages[0].setStage(::vk::ShaderStageFlagBits::eVertex)
                .setModule(vertexModule)
                .setPName("main");

    shaderStages[1].setStage(::vk::ShaderStageFlagBits::eFragment)
                .setModule(fragmentModule)
                .setPName("main");
    return shaderStages;
}

void Shader::loadGameObjects()
{
    ::std::vector<Model::Vertex> vertices{
        {{0.0, -0.5}, {1.0f, 0.0f, 0.0f}},
        {{0.5, 0.5}, {0.0f, 1.0f, 0.0f}},
        {{-0.5,0.5}, {0.0f, 0.0f, 1.0f}},
    };

    ::std::vector<::glm::vec3> colors{
        {1.f, .7f, .73f},
        {1.f, .87f, .73f},
        {1.f, 1.f, .73f},
        {.73f, 1.f, .8f},
        {.73f, .50f, 1.f}
    };
    

    for(auto& color : colors)
    {
        color = ::glm::pow(color, ::glm::vec3(2.2f));
    }
    
    auto model = ::std::make_shared<Model> (vertices);
    for(int i = 0; i < 40; i++){
        auto triangle = GameObject::createGameObject();
        triangle.model = model;
        triangle.color = colors[i % colors.size()];
        triangle.transform2d.translation.x = .2f;
        triangle.transform2d.scale = ::glm::vec2(.5f) + i * 0.025f;
        triangle.transform2d.rotation = i * ::glm::pi<float>() * .025f;
        gameObjects.push_back(::std::move(triangle));
    }

}


}