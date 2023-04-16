#pragma once

#include "g_camera.hpp"
#include "g_game_object.hpp"

#include <vulkan/vulkan.hpp>
namespace g {

    struct UniformBufferObject {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
    };

    struct FrameInfo{
        int frameIndex;
        float frameTime;
        ::vk::CommandBuffer commandBuffer;
        Camera& camera;
        ::vk::DescriptorSet descriptorSet;
        GameObject::Map& gameObjects;
    };
}