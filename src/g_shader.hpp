#ifndef G_PIPLINE_HPP
#define G_PIPLINE_HPP
#include <vulkan/vulkan.hpp>
#include <string>
#include <vector>
#include <memory>
#include "g_model.hpp"

namespace g{
    class Shader
    {
    private:
        static ::std::vector<char> readFile(const std::string& filePath);
        void createGraphicsShader(const ::std::string& vertFilePath, const ::std::string& fragFilePath);
        ::vk::ShaderModule vertexModule;
        ::vk::ShaderModule fragmentModule;
        ::std::vector<::vk::PipelineShaderStageCreateInfo> shaderStages;
        Shader(const ::std::string& vertFilePath, const ::std::string& fragFilePath);
        static ::std::unique_ptr<Shader> instance;
        ::std::unique_ptr<Model> model;
    public:
        static void init(const ::std::string& vertFilePath, const ::std::string& fragFilePath);
        static Shader& getInstance(){return *instance;}
        static void quit();
        ::vk::ShaderModule getVertex(){return vertexModule; }
        ::vk::ShaderModule getFragmentModule(){return fragmentModule; }
        ::std::vector<::vk::PipelineShaderStageCreateInfo> getShaderStage();
        void loadModel();
        Model& getModel(){return *model;}
        ~Shader();
    };
    
}

#endif