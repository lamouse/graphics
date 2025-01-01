#include "window_adapt_pass.hpp"
#include "render_core/host_shaders/vulkan_present_vert_spv.h"
#include "render_core/host_shaders/vulkan_present_frag_spv.h"
#include "render_vulkan/vk_shader_util.hpp"
namespace render::vulkan::present {
auto WindowAdaptPass::create_vertex_shader(vk::Device device) -> vk::ShaderModule {
    return utils::buildShader(device, VULKAN_PRESENT_VERT_SPV);
}
auto WindowAdaptPass::create_fragment_shader(vk::Device device) -> vk::ShaderModule {
    return utils::buildShader(device, VULKAN_PRESENT_FRAG_SPV);
}
}  // namespace render::vulkan::present