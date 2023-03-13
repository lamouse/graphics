#pragma once

#include <vulkan/vulkan.hpp>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <vector>
namespace g
{
class Model{
public:
    struct Vertex
    {
        ::glm::vec2 position;

        static ::std::vector<::vk::VertexInputBindingDescription> getBindingDescription();
        static ::std::vector<::vk::VertexInputAttributeDescription> getAtrributeDescription();
    };
    

    Model(const Model&) = delete;
    Model& operator=(const Model&) = delete;
    void bind(::vk::CommandBuffer commandBuffer);
    void draw(::vk::CommandBuffer commandBuffer);
    Model(const ::std::vector<Vertex> &vertices);
    ~Model();
private:
    ::vk::Buffer vertexBuffer;
    ::vk::DeviceMemory vertexBufferMemory;
    uint32_t vertexCount;
    void createVertexBuffers(const ::std::vector<Vertex> &vertices);
};
    
} 
