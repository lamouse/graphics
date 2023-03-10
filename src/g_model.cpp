#include "g_model.hpp"
#include "g_device.hpp"
#include <cassert>
namespace g
{

void Model::draw(::vk::CommandBuffer commandBuffer)
{
    commandBuffer.draw(vertexCount, 1, 0, 0);
}
void Model::bind(::vk::CommandBuffer commandBuffer)
{
    ::vk::Buffer buffers[] = {vertexBuffer};
    ::vk::DeviceSize offsets[] = {0};
    commandBuffer.bindVertexBuffers(0, 1, buffers, offsets);
}

void Model::createVertexBuffers(const ::std::vector<Vertex> &vertices)
{
    vertexCount = static_cast<uint32_t>(vertices.size());
    assert(vertexCount >= 3 && "Verex count must be at least 3");
    auto & device = Device::getInstance();
    ::vk::DeviceSize bufferSize = sizeof(vertices[0]) * vertexCount;

    ::vk::BufferCreateInfo bufferCreateInfo;
    bufferCreateInfo.setSize(bufferSize)
                    .setUsage(::vk::BufferUsageFlagBits::eVertexBuffer);
    vertexBuffer = device.getVKDevice().createBuffer(bufferCreateInfo);

    ::vk::MemoryRequirements memoryRequirements = device.getVKDevice().getBufferMemoryRequirements(vertexBuffer);
    ::vk::MemoryAllocateInfo allocateInfo;
    allocateInfo.setAllocationSize(memoryRequirements.size)
                .setMemoryTypeIndex(device.findMemoryType(memoryRequirements.memoryTypeBits, ::vk::MemoryPropertyFlagBits::eHostVisible|::vk::MemoryPropertyFlagBits::eHostCoherent));
    
    vertexBufferMemory = Device::getInstance().getVKDevice().allocateMemory(allocateInfo);
    Device::getInstance().getVKDevice().bindBufferMemory(vertexBuffer, vertexBufferMemory, 0);
    void* data = Device::getInstance().getVKDevice().mapMemory(vertexBufferMemory, 0, bufferSize);
    ::memcpy(data, vertices.data(), bufferSize);
    Device::getInstance().getVKDevice().unmapMemory(vertexBufferMemory);
}

Model::Model(const ::std::vector<Vertex> &vertices)
{
    createVertexBuffers(vertices);
}
Model::~Model()
{
    Device::getInstance().getVKDevice().destroyBuffer(vertexBuffer);
    Device::getInstance().getVKDevice().freeMemory(vertexBufferMemory);
}

::std::vector<::vk::VertexInputBindingDescription> Model::Vertex::getBindingDescription()
{
    ::std::vector<::vk::VertexInputBindingDescription> bindingDescriptions(1);
    bindingDescriptions[0].setBinding(0);
    bindingDescriptions[0].setStride(sizeof(Vertex));
   
   return bindingDescriptions;
}
::std::vector<::vk::VertexInputAttributeDescription> Model::Vertex::getAtrributeDescription()
{
    ::std::vector<::vk::VertexInputAttributeDescription> attributeDescriptions(1);
    attributeDescriptions[0].setBinding(0);
    attributeDescriptions[0].setLocation(0);
    attributeDescriptions[0].setFormat(::vk::Format::eR32G32Sfloat);
    attributeDescriptions[0].setOffset(0);
    return attributeDescriptions;
}

}