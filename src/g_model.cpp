#include "g_model.hpp"
#include "core/staging_buffer.hpp"
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
    ::vk::Buffer buffers[] = {vertexBuffer_()};
    ::vk::DeviceSize offsets[] = {0};
    commandBuffer.bindVertexBuffers(0, 1, buffers, offsets);
    commandBuffer.bindIndexBuffer(indexBuffer_(), 0, ::vk::IndexType::eUint16);
}

Model::Model(const ::std::vector<Vertex> &vertices, ::std::vector<uint16_t> indices, core::Device& device):
indexBuffer_(core::DeviceBuffer::create(device, ::vk::BufferUsageFlagBits::eIndexBuffer, indices.data(), sizeof(indices[0]) * indices.size())),
vertexBuffer_(core::DeviceBuffer::create(device, ::vk::BufferUsageFlagBits::eVertexBuffer, vertices.data(), sizeof(vertices[0]) * vertices.size()))
{
    vertexCount = static_cast<uint32_t>(vertices.size());
    indicesSize =  indices.size();
    assert(vertexCount >= 3 && "Verex count must be at least 3");
}
Model::~Model()
{

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
    ::std::vector<::vk::VertexInputAttributeDescription> attributeDescriptions(3);
    attributeDescriptions[0].setBinding(0);
    attributeDescriptions[0].setLocation(0);
    attributeDescriptions[0].setFormat(::vk::Format::eR32G32B32Sfloat);
    attributeDescriptions[0].setOffset(offsetof(Vertex, position));

    attributeDescriptions[1].setBinding(0);
    attributeDescriptions[1].setLocation(1);
    attributeDescriptions[1].setFormat(::vk::Format::eR32G32B32Sfloat);
    attributeDescriptions[1].setOffset(offsetof(Vertex, color));

    attributeDescriptions[2].setBinding(0);
    attributeDescriptions[2].setLocation(2);
    attributeDescriptions[2].setFormat(::vk::Format::eR32G32Sfloat);
    attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

    return attributeDescriptions;
}

}