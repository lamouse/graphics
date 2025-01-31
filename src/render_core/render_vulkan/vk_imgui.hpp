//
// Created by ziyu on 2023/11/6:0006.
//

#ifndef GRAPHICS_G_IMGUI_HPP
#define GRAPHICS_G_IMGUI_HPP
#pragma once
#include "render_core/vulkan_common/device.hpp"
#include "core/frontend/window.hpp"
#include <g_frame.hpp>
#include "render_core/uniforms.hpp"
namespace render::vulkan {
struct ImguiDebugInfo {
        float speed;
        float look_x;
        float look_y;
        float look_z;

        float center_x;
        float center_y;
        float center_z;

        float up_x;
        float up_y;
        float up_z;

        float rotate_x;
        float rotate_y;
        float rotate_z;
        float radians;

        float z_far;
        float z_near;
};
class Imgui {
    private:
        ImguiDebugInfo debugInfo{};
        void init_debug_info();
        RenderPass renderPass;
        VulkanDescriptorPool descriptorPool;
        core::frontend::BaseWindow* window;

    public:
        [[nodiscard]] auto get_uniform_buffer(float extentAspectRation) const
            -> UniformBufferObject;
        void draw(const vk::CommandBuffer& commandBuffer);
        explicit Imgui(core::frontend::BaseWindow* window, const Device& device,
                       vk::PhysicalDevice physical, vk::Instance instance, float scale = 1.0f);
        Imgui(const Imgui&) = delete;
        auto operator=(const Imgui&) -> Imgui = delete;
        auto operator=(Imgui&&) -> Imgui = delete;
        Imgui(Imgui&&) = delete;
        ~Imgui();
};

}  // namespace render::vulkan

#endif  // GRAPHICS_G_IMGUI_HPP
