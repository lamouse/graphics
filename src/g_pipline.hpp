#ifndef G_PIPLINE_HPP
#define G_PIPLINE_HPP
#include "g_device.hpp"
#include <string>
#include <vector>

namespace g{
    class Pipline
    {
    private:
        static ::std::vector<char> readFile(const std::string& filePath);
        void createGraphicsPipline(const ::std::string& vertFilePath, const ::std::string& fragFilePath);
        ::vk::ShaderModule vertexModule;
        ::vk::ShaderModule fragmentModule;
        ::std::shared_ptr<Device> device;
    public:
        Pipline(const ::std::string& vertFilePath, const ::std::string& fragFilePath, ::std::shared_ptr<Device> device);
        ~Pipline();
    };
    
}

#endif