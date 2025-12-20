#include "resource/obj/bone.hpp"
#include "resource/obj/assimp_glm_helpers.hpp"

#include <ranges>

namespace graphics {
Bone::Bone(std::string name, int id, const aiNodeAnim* channel)
    : num_positions_(static_cast<int>(channel->mNumPositionKeys)),
      num_rotations_(static_cast<int>(channel->mNumRotationKeys)),
      num_scalings_(static_cast<int>(channel->mNumScalingKeys)),
      local_transform_(1.f),
      name_(std::move(name)),
      id_(id) {
    positions_.resize(num_positions_);
    for (auto [i, position] : std::views::enumerate(positions_)) {
        position.timeStamp = static_cast<float>(channel->mPositionKeys[i].mTime);
        position.position = AssimpGLMHelpers::convert(channel->mPositionKeys[i].mValue);
    }

    rotations_.resize(num_rotations_);
    for (auto [i, rotation] : std::views::enumerate(rotations_)) {
        rotation.timeStamp = static_cast<float>(channel->mRotationKeys[i].mTime);
        rotation.orientation = AssimpGLMHelpers::convert(channel->mRotationKeys[i].mValue);
    }

    scales_.resize(num_scalings_);
    for (auto [i, scale] : std::views::enumerate(scales_)) {
        scale.timeStamp = static_cast<float>(channel->mScalingKeys[i].mTime);
        scale.scale = AssimpGLMHelpers::convert(channel->mScalingKeys[i].mValue);
    }
}

auto Bone::getScaleFactor(float lastTimeStamp, float nextTimeStamp, float animationTime) -> float {
    float scaleFactor{};
    float_t midWayLength = animationTime - lastTimeStamp;
    float framesDiff = nextTimeStamp - lastTimeStamp;
    scaleFactor = midWayLength / framesDiff;
    return scaleFactor;
}
auto Bone::interpolatePosition(float animationTime) -> glm::mat4 {
    if (positions_.size() == 1) {
        return glm::translate(glm::mat4(1.f), positions_[0].position);
    }
    int p0Index = index(animationTime, positions_);
    int p1Index = p0Index + 1;
    float scaleFactor =
        getScaleFactor(positions_[p0Index].timeStamp, positions_[p1Index].timeStamp, animationTime);
    glm::vec3 finalPosition =
        glm::mix(positions_[p0Index].position, positions_[p1Index].position, scaleFactor);
    return glm::translate(glm::mat4(1), finalPosition);
}

auto Bone::interpolateRotation(float animationTime) -> glm::mat4 {
    if (rotations_.size() == 1) {
        auto rotation = glm::normalize(rotations_[0].orientation);
        return glm::toMat4(rotation);
    }
    int p0Index = index(animationTime, rotations_);
    int p1Index = p0Index + 1;
    float scaleFactor =
        getScaleFactor(rotations_[p0Index].timeStamp, rotations_[p1Index].timeStamp, animationTime);
    glm::quat finalRotation =
        glm::slerp(rotations_[p0Index].orientation, rotations_[p1Index].orientation, scaleFactor);
    finalRotation = glm::normalize(finalRotation);
    return glm::toMat4(finalRotation);
}

auto Bone::interpolateScale(float animationTime) -> glm::mat4 {
    if (scales_.size() == 1) {
        return glm::scale(glm::mat4(1.f), scales_[0].scale);
    }
    int p0Index = index(animationTime, scales_);
    int p1Index = p0Index + 1;
    float scaleFactor =
        getScaleFactor(scales_[p0Index].timeStamp, scales_[p1Index].timeStamp, animationTime);
    glm::vec3 finalScale = glm::mix(scales_[p0Index].scale, scales_[p1Index].scale, scaleFactor);
    return glm::scale(glm::mat4(1.0f), finalScale);
}

void Bone::update(float animationTime) {
    glm::mat4 translation = interpolatePosition(animationTime);
    glm::mat4 rotation = interpolateRotation(animationTime);
    glm::mat4 scale = interpolateScale(animationTime);
    local_transform_ = translation * rotation * scale;
}
}  // namespace graphics