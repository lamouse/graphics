#pragma once

#include <vulkan/vulkan.hpp>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <vector>
namespace g
{
class Model{
public:
    struct Vertex
    {
        ::glm::vec3 position;
        ::glm::vec3 color;
        ::glm::vec2 texCoord;
        static ::std::vector<::vk::VertexInputBindingDescription> getBindingDescription();
        static ::std::vector<::vk::VertexInputAttributeDescription> getAtrributeDescription();
        bool operator==(const Vertex& other) const {
            return position == other.position && color == other.color && texCoord == other.texCoord;
        }
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

namespace std {
    template<> struct hash<g::Model::Vertex> {
        size_t operator()(g::Model::Vertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.position) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.texCoord) << 1);
        }
    };
}
