// embree_picker.hpp
#pragma once
#include <embree4/rtcore.h>
#include <glm/glm.hpp>
#include <optional>
#include <span>
#include <unordered_map>
#include "resource/id.hpp"
#include "ecs/components/transform_component.hpp"
namespace graphics {

struct PickResult;

class EmbreePicker {
    private:
        RTCDevice device_;
        RTCScene scene_;
        RTCScene main_scene_;  // 主场景（包含 instances）
        std::unordered_map<id_t, RTCGeometry> geometries_;
        std::unordered_map<unsigned int, id_t> embree_to_user;  // Embree 回调用
        std::unordered_map<id_t, RTCGeometry> instances_;       // id → instance geom

    public:
        EmbreePicker();
        ~EmbreePicker();

        // 禁止拷贝
        EmbreePicker(const EmbreePicker&) = delete;
        auto operator=(const EmbreePicker&) -> EmbreePicker& = delete;
        EmbreePicker(const EmbreePicker&&) = delete;
        auto operator=(const EmbreePicker&&) -> EmbreePicker& = delete;
        // 所有物体更新完后，一次性 commit scene
        void commit();

        // 构建三角形网格
        void buildMesh(id_t id, std::span<const glm::vec3> vertices,
                       std::span<const uint32_t> indices, bool rebuild = false);
        void updateTransform(id_t id, const ecs::TransformComponent& transform);

        auto pick(const glm::vec3& rayOrigin, const glm::vec3& rayDirection)
            -> std::optional<PickResult>;
};

}  // namespace graphics
