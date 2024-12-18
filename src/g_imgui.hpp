//
// Created by ziyu on 2023/11/6:0006.
//

#ifndef GRAPHICS_G_IMGUI_HPP
#define GRAPHICS_G_IMGUI_HPP
#pragma once

#include <GLFW/glfw3.h>

#include <chrono>
#include <g_frame.hpp>
#include <vulkan/vulkan.hpp>
namespace g{
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

    public:
        UniformBufferObject get_uniform_buffer(float extentAspectRation);
        void draw(const vk::CommandBuffer& commandBuffer);
        explicit Imgui(GLFWwindow* window, ::vk::DescriptorPool& descriptorPool, vk::RenderPass renderPass, float scale = 1.0f);
        Imgui(const Imgui&) = delete;
        auto operator=(const Imgui&) -> Imgui = delete;
        auto operator=(Imgui&&) -> Imgui = delete;
        Imgui(Imgui&&) = delete;
        ~Imgui();
    };

}

#endif  // GRAPHICS_G_IMGUI_HPP
