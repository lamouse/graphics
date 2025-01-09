#include "filters.hpp"
#include "common/common_types.hpp"
#include "render_core/host_shaders/vulkan_present_frag_spv.h"
#include "render_core/host_shaders/present_bicubic_frag_spv.h"
#include "render_core/host_shaders/present_gaussian_frag_spv.h"
#include "render_core/host_shaders/vulkan_present_frag_spv.h"
#include "render_core/host_shaders/vulkan_present_scaleforce_fp16_frag_spv.h"
#include "render_core/host_shaders/vulkan_present_scaleforce_fp32_frag_spv.h"
#include "vulkan_common/device.hpp"
#include "render_vulkan/vk_shader_util.hpp"
#include "vulkan_utils.hpp"
namespace render::vulkan {
ShaderModule SelectScaleForceShader(const Device& device) {
    if (device.isFloat16Supported()) {
        return utils::buildShader(device.getLogical(), VULKAN_PRESENT_SCALEFORCE_FP16_FRAG_SPV);
    }
    return utils::buildShader(device.getLogical(), VULKAN_PRESENT_SCALEFORCE_FP32_FRAG_SPV);

}  // anonymous namespace

std::unique_ptr<present::WindowAdaptPass> MakeNearestNeighbor(const Device& device,
                                                              vk::Format frame_format) {
    return std::make_unique<present::WindowAdaptPass>(
        device, frame_format, present::utils::CreateNearestNeighborSampler(device),
        utils::buildShader(device.getLogical(), VULKAN_PRESENT_FRAG_SPV));
}

std::unique_ptr<present::WindowAdaptPass> MakeBilinear(const Device& device,
                                                       vk::Format frame_format) {
    return std::make_unique<present::WindowAdaptPass>(
        device, frame_format, present::utils::CreateBilinearSampler(device),
        utils::buildShader(device.getLogical(), VULKAN_PRESENT_FRAG_SPV));
}

std::unique_ptr<present::WindowAdaptPass> MakeBicubic(const Device& device,
                                                      vk::Format frame_format) {
    return std::make_unique<present::WindowAdaptPass>(
        device, frame_format, present::utils::CreateBilinearSampler(device),
        utils::buildShader(device.getLogical(), PRESENT_BICUBIC_FRAG_SPV));
}

std::unique_ptr<present::WindowAdaptPass> MakeGaussian(const Device& device,
                                                       vk::Format frame_format) {
    return std::make_unique<present::WindowAdaptPass>(
        device, frame_format, present::utils::CreateBilinearSampler(device),
        utils::buildShader(device.getLogical(), PRESENT_GAUSSIAN_FRAG_SPV));
}

std::unique_ptr<present::WindowAdaptPass> MakeScaleForce(const Device& device,
                                                         vk::Format frame_format) {
    return std::make_unique<present::WindowAdaptPass>(device, frame_format,
                                                      present::utils::CreateBilinearSampler(device),
                                                      SelectScaleForceShader(device));
}
}  // namespace render::vulkan