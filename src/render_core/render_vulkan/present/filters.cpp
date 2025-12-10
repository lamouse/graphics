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
auto SelectScaleForceShader(const Device& device) -> ShaderModule {
    if (device.isFloat16Supported()) {
        return utils::buildShader(device.getLogical(), VULKAN_PRESENT_SCALEFORCE_FP16_FRAG_SPV);
    }
    return utils::buildShader(device.getLogical(), VULKAN_PRESENT_SCALEFORCE_FP32_FRAG_SPV);

}  // anonymous namespace

auto MakeNearestNeighbor(const Device& device, vk::Format frame_format)
    -> std::unique_ptr<present::WindowAdaptPass> {
    return std::make_unique<present::WindowAdaptPass>(
        device, frame_format, present::utils::CreateNearestNeighborSampler(device),
        utils::buildShader(device.getLogical(), VULKAN_PRESENT_FRAG_SPV));
}

auto MakeBilinear(const Device& device, vk::Format frame_format)
    -> std::unique_ptr<present::WindowAdaptPass> {
    return std::make_unique<present::WindowAdaptPass>(
        device, frame_format, present::utils::CreateBilinearSampler(device),
        utils::buildShader(device.getLogical(), VULKAN_PRESENT_FRAG_SPV));
}

auto MakeBicubic(const Device& device, vk::Format frame_format)
    -> std::unique_ptr<present::WindowAdaptPass> {
    return std::make_unique<present::WindowAdaptPass>(
        device, frame_format, present::utils::CreateBilinearSampler(device),
        utils::buildShader(device.getLogical(), PRESENT_BICUBIC_FRAG_SPV));
}

auto MakeGaussian(const Device& device, vk::Format frame_format)
    -> std::unique_ptr<present::WindowAdaptPass> {
    return std::make_unique<present::WindowAdaptPass>(
        device, frame_format, present::utils::CreateBilinearSampler(device),
        utils::buildShader(device.getLogical(), PRESENT_GAUSSIAN_FRAG_SPV));
}

auto MakeScaleForce(const Device& device, vk::Format frame_format)
    -> std::unique_ptr<present::WindowAdaptPass> {
    return std::make_unique<present::WindowAdaptPass>(device, frame_format,
                                                      present::utils::CreateBilinearSampler(device),
                                                      SelectScaleForceShader(device));
}
}  // namespace render::vulkan