#include "g_pipline.hpp"
#include <fstream>
#include <stdexcept>
#include <iostream>
namespace g{

Pipline::Pipline(const ::std::string& vertFilePath, const ::std::string& fragFilePath)
{
    createGraphicsPipline(vertFilePath, fragFilePath);
}

Pipline::~Pipline()
{
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
}
}