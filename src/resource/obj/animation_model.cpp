#include "resource/obj/animation_model.hpp"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <stdexcept>
namespace graphics::animation {

void Model::loadModel(std::string_view path) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path.data(),  aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace);
        if (!scene || !scene->HasMeshes() || !scene->mRootNode ||
        scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) {
        throw std::runtime_error("load model fail: " + std::string(importer.GetErrorString()));
    }
}

}  // namespace graphics::animation