#include "resource/obj/animation_model.hpp"
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

auto Model::processMesh(aiMesh* mesh, const aiScene* scene) -> Mesh{
    std::vector<Vertex> vertices;
    std::vector<std::uint32_t> indices;
    MeshMaterial material;
}

}  // namespace graphics::animation