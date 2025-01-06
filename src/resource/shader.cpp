#include "shader.hpp"

#include <spdlog/spdlog.h>

#include <fstream>
#include <stdexcept>
#include "render_core/render_vulkan/present/window_adapt_pass.hpp"

namespace resource::shader {

auto readShaderFile(const std::string& filePath) -> ::std::vector<char> {
    ::std::ifstream file{filePath, ::std::ios::ate | ::std::ios::binary};
    if (!file.is_open()) {
        SPDLOG_ERROR("failed to open file: {}", filePath);
        throw ::std::runtime_error("failed to open file: " + filePath);
    }
    auto fileSize = file.tellg();
    ::std::vector<char> buffer(static_cast<size_t>(fileSize));
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    return buffer;
}

GraphicsShader::GraphicsShader(const ::std::string& vertFilePath, const ::std::string& fragFilePath,
                               ::vk::Device& device)
    : device_(device) {
    createGraphicsShader(vertFilePath, fragFilePath);
}

GraphicsShader::~GraphicsShader() {
    device_.destroyShaderModule(vertexModule);
    device_.destroyShaderModule(fragmentModule);
}

void GraphicsShader::createGraphicsShader(const ::std::string& vertFilePath,
                                          const ::std::string& fragFilePath) {
    auto vertCode = readShaderFile(vertFilePath);
    auto fragCode = readShaderFile(fragFilePath);
    ::vk::ShaderModuleCreateInfo createInfo;
    createInfo.setCodeSize(vertCode.size());
    createInfo.setPCode((uint32_t*)vertCode.data());
    render::vulkan::present::WindowAdaptPass windowAdaptPass;
    vertexModule = device_.createShaderModule(createInfo);

    createInfo.setCodeSize(fragCode.size());
    createInfo.setPCode((uint32_t*)fragCode.data());
    fragmentModule = device_.createShaderModule(createInfo);
}

auto GraphicsShader::getShaderStages() -> ::std::vector<::vk::PipelineShaderStageCreateInfo> {
    ::std::vector<::vk::PipelineShaderStageCreateInfo> shaderStages;
    shaderStages.resize(2);
    shaderStages[0]
        .setStage(::vk::ShaderStageFlagBits::eVertex)
        .setModule(vertexModule)
        .setPName("main");

    shaderStages[1]
        .setStage(::vk::ShaderStageFlagBits::eFragment)
        .setModule(fragmentModule)
        .setPName("main");
    return shaderStages;
}

ComputeShader::ComputeShader(::vk::Device& device, const ::std::string& filePath)
    : device_(device) {
    createComputeShader(filePath);
}

void ComputeShader::createComputeShader(const ::std::string& filePath) {
    auto computeCode = readShaderFile(filePath);
    ::vk::ShaderModuleCreateInfo createInfo;
    createInfo.setCodeSize(computeCode.size());
    createInfo.setPCode((uint32_t*)computeCode.data());
    computeModule = device_.createShaderModule(createInfo);
}

ComputeShader::~ComputeShader() { device_.destroyShaderModule(computeModule); }
auto ComputeShader::getShaderStages() -> ::vk::PipelineShaderStageCreateInfo {
    return {
        {},
        ::vk::ShaderStageFlagBits::eCompute,
        computeModule,
        "main",
    };
}

}  // namespace resource::shader