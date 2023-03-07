#include "g_pipline.hpp"
#include <fstream>
#include <stdexcept>
#include <iostream>
namespace g{

Pipline::Pipline(const ::std::string& vertFilePath, const ::std::string& fragFilePath, ::std::shared_ptr<Device> device):
                                                                                device{device}
{
    createGraphicsPipline(vertFilePath, fragFilePath);
}

Pipline::~Pipline()
{
    device->getVKDevice().destroyShaderModule(vertexModule);
    device->getVKDevice().destroyShaderModule(fragmentModule);
}

::std::vector<char> Pipline::readFile(const std::string& filePath){
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

void Pipline::createGraphicsPipline(const ::std::string& vertFilePath, const ::std::string& fragFilePath){
    auto vertCode = readFile(vertFilePath);
    auto fragCode = readFile(fragFilePath);
    ::std::cout << "Vertex Size " << vertCode.size() << "\n";
    ::std::cout << "Frag Size " << fragCode.size() << "\n";
    ::vk::ShaderModuleCreateInfo createInfo;
    createInfo.setCodeSize(vertCode.size());
    createInfo.setPCode((uint32_t*)vertCode.data());
    vertexModule = device->getVKDevice().createShaderModule(createInfo);

    createInfo.setCodeSize(fragCode.size());
    createInfo.setPCode((uint32_t*)fragCode.data());
    fragmentModule = device->getVKDevice().createShaderModule(createInfo);

}


}