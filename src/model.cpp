#include "model.hpp"

#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <assimp/Importer.hpp>
#include <cassert>
#include <format>
#include <unordered_map>

namespace graphics {


Model::Model(const ::std::vector<Vertex>& vertices, const ::std::vector<uint16_t>& indices)
    : vertices_(vertices),
      indices_(indices),
      vertexCount(static_cast<uint32_t>(vertices.size())),
      indicesSize(static_cast<uint32_t>(indices.size())) {


    assert(vertexCount >= 3 && "Vertex count must be at least 3");
}

auto Model::createFromFile(const ::std::string& path) -> ::std::unique_ptr<Model> {
    std::vector<Model::Vertex> vertices;
    std::vector<uint16_t> indices;

    ::std::unordered_map<Model::Vertex, uint32_t> uniqueVertices{};

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);
    if (!scene) {
        throw std::runtime_error(
            std::format("Failed to load model: {}", importer.GetErrorString()));
    }
    for (unsigned int i = 0; i < scene->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[i];
        // auto color = mesh->GetNumColorChannels();
        auto texCoord = mesh->GetNumUVChannels();
        for (unsigned int j = 0; j < mesh->mNumVertices; j++) {
            aiVector3D vertex = mesh->mVertices[j];
            Model::Vertex model_vertex{};
            model_vertex.position = {vertex.x, vertex.y, vertex.z};
            if (texCoord > 0) {
                model_vertex.texCoord = {mesh->mTextureCoords[0][j].x,
                                         mesh->mTextureCoords[0][j].y};
            } else {
                model_vertex.texCoord = {0.0f, 0.0f};
            }
            model_vertex.color = {1.0f, 1.0f, 1.0f};
            if (!uniqueVertices.contains(model_vertex)) {
                uniqueVertices[model_vertex] = static_cast<uint32_t>(vertices.size());
                vertices.push_back(model_vertex);
            }

            indices.push_back(
                static_cast<decltype(indices)::value_type>(uniqueVertices[model_vertex]));
        }
    }
    return std::make_unique<Model>(vertices, indices);
}

}  // namespace g
