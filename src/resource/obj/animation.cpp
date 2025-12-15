#include "resource/obj/animation.hpp"
#include "resource/obj/assimp_glm_helpers.hpp"
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <stdexcept>
#include <stack>
namespace graphics::animation {
Animation::Animation(std::string_view animationPath) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(animationPath.data(), aiProcess_Triangulate);
    if (!scene || !scene->HasMeshes() || !scene->mRootNode ||
        scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) {
        throw std::runtime_error("load model fail: " + std::string(importer.GetErrorString()));
    }
    auto* animation = scene->mAnimations[0];
    duration_ = static_cast<float>(animation->mDuration);
    ticks_per_second = static_cast<int>(animation->mTicksPerSecond);
    aiMatrix4x4 globalTransformation = scene->mRootNode->mTransformation;
    globalTransformation = globalTransformation.Inverse();
}

void Animation::readMissingBones(const aiAnimation* animation) {}
void Animation::readHierarchyData(AssimpNodeDate& rootDest, const aiNode* rootSrc) {
    assert(rootSrc);

    // 映射关系：srcNode -> destNode 指针
    struct NodePair {
            AssimpNodeDate* dest;
            const aiNode* src;
    };

    rootDest.name = rootSrc->mName.C_Str();
    rootDest.transformation = AssimpGLMHelpers::convert(rootSrc->mTransformation);
    rootDest.childrenCount = static_cast<int>(rootSrc->mNumChildren);

    std::stack<NodePair> stack;
    stack.push({&rootDest, rootSrc});

    while (!stack.empty()) {
        NodePair current = stack.top();
        stack.pop();

        AssimpNodeDate* dest = current.dest;
        const aiNode* src = current.src;
        dest->childrenCount = static_cast<int>(src->mNumChildren);
        dest->children.reserve(dest->childrenCount);

        for (unsigned int i = 0; i < src->mNumChildren; ++i) {
            const aiNode* childSrc = src->mChildren[i];
            AssimpNodeDate childDest;
            childDest.name = childSrc->mName.C_Str();
            childDest.transformation = AssimpGLMHelpers::convert(childSrc->mTransformation);
            dest->children.push_back(std::move(childDest));
            stack.push({&dest->children.back(), childSrc});
        }
    }
}
}  // namespace graphics::animation