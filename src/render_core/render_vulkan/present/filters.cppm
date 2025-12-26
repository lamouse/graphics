module;
#include <memory>
#include <vulkan/vulkan.hpp>
export module render.vulkan:filters;
import render.vulkan.common;
import :window_adapt_pass;
export  namespace render::vulkan {

auto MakeNearestNeighbor(const Device& device, vk::Format frame_format)
    -> std::unique_ptr<present::WindowAdaptPass>;
auto MakeBilinear(const Device& device, vk::Format frame_format)
    -> std::unique_ptr<present::WindowAdaptPass>;
auto MakeBicubic(const Device& device, vk::Format frame_format)
    -> std::unique_ptr<present::WindowAdaptPass>;
auto MakeGaussian(const Device& device, vk::Format frame_format)
    -> std::unique_ptr<present::WindowAdaptPass>;
auto MakeScaleForce(const Device& device, vk::Format frame_format)
    -> std::unique_ptr<present::WindowAdaptPass>;

}  // namespace render::vulkan