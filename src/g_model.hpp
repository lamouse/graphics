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
        ::glm::vec3 color;

        static ::std::vector<::vk::VertexInputBindingDescription> getBindingDescription();
        static ::std::vector<::vk::VertexInputAttributeDescription> getAtrributeDescription();
    };
    

    Model(const Model&) = delete;
    Model& operator=(const Model&) = delete;
    void bind(::vk::CommandBuffer commandBuffer);
    void draw(::vk::CommandBuffer commandBuffer);
    Model(const ::std::vector<Vertex> &vertices, ::std::vector<uint16_t> indices);
    ~Model();
private:
    ::vk::Buffer vertexBuffer;
    ::vk::DeviceMemory vertexBufferMemory;
    ::vk::Buffer indexBuffer;
    ::vk::DeviceMemory indexBufferMemory;
    uint32_t vertexCount;
    uint32_t indicesSize;
    void createVertexBuffers(const ::std::vector<Vertex> &vertices);
    void createIndexBuffer(::std::vector<uint16_t>& indices);
    void copyBuffer(::vk::Buffer srcBuffer, vk::Buffer dstBuffer, ::vk::DeviceSize size);
};
    
} 
