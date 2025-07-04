#pragma once

#include <vulkan/vulkan.hpp>

#include "g_game_object.hpp"

namespace graphics {

struct UniformBufferObject {
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
};

struct FrameInfo {
        uint32_t frameIndex;
        ::vk::CommandBuffer commandBuffer;
        ::vk::DescriptorSet descriptorSet;
        GameObject::Map& gameObjects;
};
}  // namespace graphics