#include "g_model.hpp"
#include <cassert>
#include <unordered_map>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
namespace g
{

void Model::draw(const ::vk::CommandBuffer& commandBuffer)const
{
    commandBuffer.drawIndexed(indicesSize, 1, 0, 0, 0);
}
void Model::bind(const ::vk::CommandBuffer& commandBuffer)
{
    ::std::array<::vk::Buffer, 1> buffers = {vertexBuffer_()};
    ::std::array<::vk::DeviceSize, 1> offsets = {0};
    commandBuffer.bindVertexBuffers(0, buffers.size(), buffers.data(), offsets.data());
    commandBuffer.bindIndexBuffer(indexBuffer_(), 0, ::vk::IndexType::eUint16);
}

Model::Model(const ::std::vector<Vertex> &vertices, ::std::vector<uint16_t> indices, core::Device& device):
vertexBuffer_(core::DeviceBuffer::create(device, ::vk::BufferUsageFlagBits::eVertexBuffer, vertices.data(), sizeof(vertices[0]) * vertices.size())),
indexBuffer_(core::DeviceBuffer::create(device, ::vk::BufferUsageFlagBits::eIndexBuffer, indices.data(), sizeof(indices[0]) * indices.size()))
{
    vertexCount = static_cast<uint32_t>(vertices.size());
    indicesSize =  indices.size();
    assert(vertexCount >= 3 && "Verex count must be at least 3");
}

auto Model::Vertex::getBindingDescription() -> ::std::vector<::vk::VertexInputBindingDescription>
{
    ::std::vector<::vk::VertexInputBindingDescription> bindingDescriptions(1);
    bindingDescriptions[0].setBinding(0);
    bindingDescriptions[0].setStride(sizeof(Vertex));
   
   return bindingDescriptions;
}
auto Model::Vertex::getAttributeDescription() -> ::std::vector<::vk::VertexInputAttributeDescription>
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

auto Model::createFromFile(const ::std::string& path, core::Device& device) -> ::std::unique_ptr<Model>
{
    ::tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str())) {
        throw std::runtime_error(warn + err);
    }
    std::vector<Model::Vertex> vertices;
    std::vector<uint16_t> indices;
   
   ::std::unordered_map<Model::Vertex, uint32_t> uniqueVertices{};

        for (const auto& shape : shapes) {
            for (const auto& index : shape.mesh.indices) {
                Model::Vertex vertex{};

                vertex.position = {
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]
                };

                vertex.texCoord = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
                };

                vertex.color = {1.0f, 1.0f, 1.0f};

                if (!uniqueVertices.contains(vertex)) {
                    uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                    vertices.push_back(vertex);
                }

                indices.push_back(uniqueVertices[vertex]);
            }
        }
     
  return std::make_unique<Model>(vertices, indices, device);
}

}