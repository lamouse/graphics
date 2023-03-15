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
    loadGameObjects();
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

std::unique_ptr<Model> createCubeModel(::glm::vec3 offset) {
  std::vector<Model::Vertex> vertices{
 
      // left face (white)
      {{-.5f, -.5f, -.5f}, {.9f, .9f, .9f}},
      {{-.5f, .5f, .5f}, {.9f, .9f, .9f}},
      {{-.5f, -.5f, .5f}, {.9f, .9f, .9f}},
      {{-.5f, -.5f, -.5f}, {.9f, .9f, .9f}},
      {{-.5f, .5f, -.5f}, {.9f, .9f, .9f}},
      {{-.5f, .5f, .5f}, {.9f, .9f, .9f}},
 
      // right face (yellow)
      {{.5f, -.5f, -.5f}, {.8f, .8f, .1f}},
      {{.5f, .5f, .5f}, {.8f, .8f, .1f}},
      {{.5f, -.5f, .5f}, {.8f, .8f, .1f}},
      {{.5f, -.5f, -.5f}, {.8f, .8f, .1f}},
      {{.5f, .5f, -.5f}, {.8f, .8f, .1f}},
      {{.5f, .5f, .5f}, {.8f, .8f, .1f}},
 
      // top face (orange, remember y axis points down)
      {{-.5f, -.5f, -.5f}, {.9f, .6f, .1f}},
      {{.5f, -.5f, .5f}, {.9f, .6f, .1f}},
      {{-.5f, -.5f, .5f}, {.9f, .6f, .1f}},
      {{-.5f, -.5f, -.5f}, {.9f, .6f, .1f}},
      {{.5f, -.5f, -.5f}, {.9f, .6f, .1f}},
      {{.5f, -.5f, .5f}, {.9f, .6f, .1f}},
 
      // bottom face (red)
      {{-.5f, .5f, -.5f}, {.8f, .1f, .1f}},
      {{.5f, .5f, .5f}, {.8f, .1f, .1f}},
      {{-.5f, .5f, .5f}, {.8f, .1f, .1f}},
      {{-.5f, .5f, -.5f}, {.8f, .1f, .1f}},
      {{.5f, .5f, -.5f}, {.8f, .1f, .1f}},
      {{.5f, .5f, .5f}, {.8f, .1f, .1f}},
 
      // nose face (blue)
      {{-.5f, -.5f, 0.5f}, {.1f, .1f, .8f}},
      {{.5f, .5f, 0.5f}, {.1f, .1f, .8f}},
      {{-.5f, .5f, 0.5f}, {.1f, .1f, .8f}},
      {{-.5f, -.5f, 0.5f}, {.1f, .1f, .8f}},
      {{.5f, -.5f, 0.5f}, {.1f, .1f, .8f}},
      {{.5f, .5f, 0.5f}, {.1f, .1f, .8f}},
 
      // tail face (green)
      {{-.5f, -.5f, -0.5f}, {.1f, .8f, .1f}},
      {{.5f, .5f, -0.5f}, {.1f, .8f, .1f}},
      {{-.5f, .5f, -0.5f}, {.1f, .8f, .1f}},
      {{-.5f, -.5f, -0.5f}, {.1f, .8f, .1f}},
      {{.5f, -.5f, -0.5f}, {.1f, .8f, .1f}},
      {{.5f, .5f, -0.5f}, {.1f, .8f, .1f}},
 
  };
  for (auto& v : vertices) {
    v.position += offset;
  }
  return std::make_unique<Model>(vertices);
}

void Shader::loadGameObjects()
{
 
    ::std::shared_ptr<Model> model = createCubeModel({.0f, .0f, .0f});
    auto cube = GameObject::createGameObject();
    cube.model = model;
    cube.transform.translation = {0.f,.0f, .5f};
    cube.transform.scale = {.5f, .5f, .5f};
    gameObjects.push_back(std::move(cube));
}


}