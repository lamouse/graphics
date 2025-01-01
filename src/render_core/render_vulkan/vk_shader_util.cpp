#include "vk_shader_util.hpp"
namespace render::vulkan::utils {
auto buildShader(vk::Device device, std::span<const uint32_t> code) -> vk::ShaderModule {
    return device.createShaderModule(
        vk::ShaderModuleCreateInfo({}, code.size_bytes(), code.data()));
}
}  // namespace render::vulkan::utils