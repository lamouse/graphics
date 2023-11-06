//
// Created by ziyu on 2023/11/6:0006.
//

#ifndef GRAPHICS_G_IMGUI_HPP
#define GRAPHICS_G_IMGUI_HPP
#pragma once

#include <GLFW/glfw3.h>

#include "core/device.hpp"
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
        ::VkRenderPass renderPass;
        ::vk::Device device_;
        void createRenderPass();
    public:
        void init(GLFWwindow* window, core::Device& device, ::vk::DescriptorPool& descriptorPool, float scale = 1.0f);
        void draw(ImguiDebugInfo& debugInfo);
        Imgui(core::Device& device):device_(device.logicalDevice()){createRenderPass();};
        ~Imgui();
    };

}

#endif  // GRAPHICS_G_IMGUI_HPP
