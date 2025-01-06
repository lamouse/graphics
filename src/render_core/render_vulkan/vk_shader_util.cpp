#include "vk_shader_util.hpp"
namespace render::vulkan::utils {
auto buildShader(vk::Device device, std::span<const uint32_t> code) -> ShaderModule {
    return ShaderModule{
        device.createShaderModule(vk::ShaderModuleCreateInfo({}, code.size_bytes(), code.data())),
        device};
}
}  // namespace render::vulkan::utils