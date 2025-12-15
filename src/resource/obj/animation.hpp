#pragma once
#include "resource/obj/bone_info.hpp"
#include <glm/glm.hpp>
#include <resource/obj/bone.hpp>
#include <string_view>
#include <map>

namespace graphics::animation {

struct AssimpNodeDate {
        glm::mat4 transformation;
        std::string name;
        int childrenCount;
        std::vector<AssimpNodeDate> children;
};

class Animation {
    public:
        Animation() = default;
        ~Animation() = default;
        Animation(std::string_view path);

    private:
        void readMissingBones(const aiAnimation* animation);
        void readHierarchyData(AssimpNodeDate& rootDest, const aiNode* rootSrc);
        float duration_;
        int ticks_per_second;
        std::vector<Bone> bones_;
        std::map<std::string, BoneInfo> boneInfoMap;
};

}  // namespace graphics::animation