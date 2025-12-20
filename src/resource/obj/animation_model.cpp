#include "resource/obj/animation_model.hpp"
#include "resource/obj/assimp_glm_helpers.hpp"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <stdexcept>
#include <stack>
namespace graphics::animation {

void Model::loadModel(std::string_view path) {
    Assimp::Importer importer;
    const aiScene* scene =
        importer.ReadFile(path.data(), aiProcess_Triangulate | aiProcess_GenSmoothNormals |
                                           aiProcess_CalcTangentSpace);
    if (!scene || !scene->HasMeshes() || !scene->mRootNode ||
        scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) {
        throw std::runtime_error("load model fail: " + std::string(importer.GetErrorString()));
    }
    processNode(scene->mRootNode, scene);
}

void Model::processNode(aiNode* root_node, const aiScene* scene) {
    std::stack<aiNode*> stack;
    stack.push(root_node);

    while (!stack.empty()) {
        auto* node = stack.top();
        stack.pop();

        // 处理该节点的所有网格
        for (unsigned int i = 0; i < node->mNumMeshes; i++) {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            processMesh(mesh, scene);
        }

        // 将子节点压栈
        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            stack.push(node->mChildren[i]);
        }
    }
}

void Model::processMesh(aiMesh* mesh, const aiScene* scene) {
    std::vector<Vertex> vertices;
    std::vector<glm::vec3> positions;
    vertices.reserve(mesh->mNumVertices);
    std::vector<std::uint32_t> indices;
    MeshMaterial material = loadMaterial(scene, mesh);
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex{};
        vertex.position =
            glm::vec3{mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z};
        if (mesh->HasNormals()) {
            vertex.normal =
                glm::vec3{mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z};
        }
        if (mesh->HasTextureCoords(0)) {
            vertex.texCoords =
                glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
        }
        extract_bone_weight(vertices, mesh);
        vertices.push_back(vertex);
    }

    meshes_.emplace_back(vertices, positions, indices, material);
}

void Model::extract_bone_weight(std::vector<graphics::animation::Vertex>& vertices, aiMesh* mesh) {
    for (unsigned int boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex) {
        auto* bone = mesh->mBones[boneIndex];
        int boneID = -1;
        std::string boneName = bone->mName.C_Str();
        const auto [pair, is_new] = bone_info_map_.try_emplace(boneName);
        if (is_new) {
            BoneInfo newBoneInfo{};
            newBoneInfo.id = bone_counter_;
            newBoneInfo.offset = AssimpGLMHelpers::convert(bone->mOffsetMatrix);
            pair->second = newBoneInfo;
            boneID = bone_counter_;
            bone_counter_++;
        } else {
            boneID = pair->second.id;
        }
        assert(boneID != -1);
        auto* weights = bone->mWeights;
        auto numWeights = bone->mNumWeights;
        for (unsigned int weightIndex = 0; weightIndex < numWeights; weightIndex++) {
            unsigned int vertexId = weights[weightIndex].mVertexId;
            float weight = weights[weightIndex].mWeight;
            assert(vertexId <= vertices.size());
            vertices[vertexId].setBone(boneID, weight);
        }
    }
}

}  // namespace graphics::animation