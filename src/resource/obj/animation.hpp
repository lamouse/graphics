#pragma once
#include "resource/obj/bone_info.hpp"
#include "resource/obj/animation_model.hpp"
#include <algorithm>
#include <glm/glm.hpp>
#include <resource/obj/bone.hpp>
#include <string_view>
#include <map>

namespace graphics::animation {

struct AssimpNodeData {
        glm::mat4 transformation;
        std::string name;
        int childrenCount;
        std::vector<AssimpNodeData> children;
};

class Animation {
    public:
        Animation() = default;
        ~Animation() = default;
        Animation(const Animation&) = delete;
        Animation(Animation&&) noexcept = delete;
        auto operator=(const Animation&) -> Animation& = delete;
        auto operator=(Animation&&) noexcept -> Animation& = delete;
        Animation(std::string_view path, Model* model);
        auto findBone(const std::string_view name) -> Bone* {
            auto iter =
                std::ranges::find_if(bones_, [&](const Bone& bone) { return bone.Name() == name; });
            if (iter == bones_.end()) {
                return nullptr;
            }
            return &(*iter);
        }

        [[nodiscard]] inline auto getTicksPerSecond() const -> float {
            return static_cast<float>(ticks_per_second);
        }
        inline auto getRootNode() -> const AssimpNodeData& { return root_node_; }
        inline auto GetBoneIDMap() -> const std::map<std::string, BoneInfo>& {
            return boneInfoMap_;
        }

    private:
        void readMissingBones(const aiAnimation* animation, Model& model);
        void readHierarchyData(AssimpNodeData& rootDest, const aiNode* rootSrc);
        float duration_{};
        int ticks_per_second{};
        std::vector<Bone> bones_;
        AssimpNodeData root_node_;
        std::map<std::string, BoneInfo> boneInfoMap_;
};

}  // namespace graphics::animation