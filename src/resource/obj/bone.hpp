#pragma once
#include <assimp/scene.h>
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <vector>
#include <string>
namespace graphics {
struct KeyPosition {
        glm::vec3 position;
        float timeStamp;
};
struct KeyRotation {
        glm::quat orientation;
        float timeStamp;
};

struct KeyScale {
        glm::vec3 scale;
        float timeStamp;
};

class Bone {
    public:
        Bone(std::string name, int id, const aiNodeAnim* channel);
        void update(float animationTime);
        [[nodiscard]] auto Name() const -> std::string_view { return name_; }

    private:
        template <typename Container>
        auto index(float animationTime, const Container& keys) -> int {
            for (int i = 0; i < keys.size() - 1; i++) {
                if (animationTime < keys[i].timeStamp) {
                    return i;
                }
            }
            assert(0);
        }

        auto getScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime) -> float;
        auto interpolatePosition(float animationTime) -> glm::mat4;
        auto interpolateRotation(float animationTime) -> glm::mat4;
        auto interpolateScale(float animationTime) -> glm::mat4;
        auto transform() -> glm::mat4 { return local_transform_; }
        auto getBoneName() -> std::string { return name_; }
        [[nodiscard]] auto getBoneID() const -> int { return id_; }

        std::vector<KeyPosition> positions_;
        std::vector<KeyRotation> rotations_;
        std::vector<KeyScale> scales_;
        int num_positions_{};
        int num_rotations_{};
        int num_scalings_{};

        glm::mat4 local_transform_;
        std::string name_;

        int id_;
};
}  // namespace graphics