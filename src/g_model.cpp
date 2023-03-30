#include "g_model.hpp"
#include "g_device.hpp"
#include <cassert>
namespace g
{

void Model::draw(::vk::CommandBuffer commandBuffer)
{
    //commandBuffer.draw(vertexCount, 1, 0, 0);
    commandBuffer.drawIndexed(indicesSize, 1, 0, 0, 0);
}
void Model::bind(::vk::CommandBuffer commandBuffer)
{
    ::vk::Buffer buffers[] = {vertexBuffer};
    ::vk::DeviceSize offsets[] = {0};
    commandBuffer.bindVertexBuffers(0, 1, buffers, offsets);
    commandBuffer.bindIndexBuffer(indexBuffer, 0, ::vk::IndexType::eUint16);
}

void Model::createVertexBuffers(const ::std::vector<Vertex> &vertices)
{
    vertexCount = static_cast<uint32_t>(vertices.size());
    assert(vertexCount >= 3 && "Verex count must be at least 3");
    auto & device = Device::getInstance();
    ::vk::DeviceSize bufferSize = sizeof(vertices[0]) * vertexCount;

    ::vk::Buffer stagingBuffer;
    ::vk::DeviceMemory stagingBufferMemory;
    Device::getInstance().createBuffer(bufferSize, ::vk::BufferUsageFlagBits::eTransferSrc, 
                ::vk::MemoryPropertyFlagBits::eHostVisible|::vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer, stagingBufferMemory);
    void* data = Device::getInstance().getVKDevice().mapMemory(stagingBufferMemory, 0, bufferSize);
    ::memcpy(data, vertices.data(), bufferSize);
    Device::getInstance().getVKDevice().unmapMemory(stagingBufferMemory);

    Device::getInstance().createBuffer(bufferSize, ::vk::BufferUsageFlagBits::eVertexBuffer | ::vk::BufferUsageFlagBits::eTransferDst, 
                ::vk::MemoryPropertyFlagBits::eDeviceLocal|::vk::MemoryPropertyFlagBits::eHostCoherent, vertexBuffer,vertexBufferMemory);
    copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

    Device::getInstance().getVKDevice().destroyBuffer(stagingBuffer);
    Device::getInstance().getVKDevice().freeMemory(stagingBufferMemory);
}

void Model::createIndexBuffer(::std::vector<uint16_t>& indices) {
    indicesSize = indices.size();
    VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();
    ::vk::Buffer stagingBuffer;
    ::vk::DeviceMemory stagingBufferMemory;
    Device::getInstance().createBuffer(bufferSize, ::vk::BufferUsageFlagBits::eTransferSrc, 
                                        ::vk::MemoryPropertyFlagBits::eHostVisible|::vk::MemoryPropertyFlagBits::eHostCoherent, stagingBuffer, stagingBufferMemory);

    void* data = Device::getInstance().getVKDevice().mapMemory(stagingBufferMemory, 0, bufferSize);
    memcpy(data, indices.data(), (size_t) bufferSize);
    Device::getInstance().getVKDevice().unmapMemory(stagingBufferMemory);

    Device::getInstance().createBuffer(bufferSize, ::vk::BufferUsageFlagBits::eIndexBuffer | ::vk::BufferUsageFlagBits::eTransferDst, 
                                        ::vk::MemoryPropertyFlagBits::eDeviceLocal|::vk::MemoryPropertyFlagBits::eHostCoherent, indexBuffer, indexBufferMemory);

    copyBuffer(stagingBuffer, indexBuffer, bufferSize);

    Device::getInstance().getVKDevice().destroyBuffer(stagingBuffer);
    Device::getInstance().getVKDevice().freeMemory(stagingBufferMemory);
}

Model::Model(const ::std::vector<Vertex> &vertices, ::std::vector<uint16_t> indices)
{
    createVertexBuffers(vertices);
    createIndexBuffer(indices);
}
Model::~Model()
{
    Device::getInstance().getVKDevice().destroyBuffer(indexBuffer);
    Device::getInstance().getVKDevice().freeMemory(indexBufferMemory);
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
    ::std::vector<::vk::VertexInputAttributeDescription> attributeDescriptions(2);
    attributeDescriptions[0].setBinding(0);
    attributeDescriptions[0].setLocation(0);
    attributeDescriptions[0].setFormat(::vk::Format::eR32G32Sfloat);
    attributeDescriptions[0].setOffset(offsetof(Vertex, position));

    attributeDescriptions[1].setBinding(0);
    attributeDescriptions[1].setLocation(1);
    attributeDescriptions[1].setFormat(::vk::Format::eR32G32B32Sfloat);
    attributeDescriptions[1].setOffset(offsetof(Vertex, color));
    return attributeDescriptions;
}


void Model::copyBuffer(::vk::Buffer srcBuffer, vk::Buffer dstBuffer, ::vk::DeviceSize size)
{
    Device::getInstance().excuteCmd([&](auto& cmdBuf){
        ::vk::BufferCopy copyRegion{};
        copyRegion.size = size;
        cmdBuf.copyBuffer(srcBuffer, dstBuffer, copyRegion);
    });

}

}