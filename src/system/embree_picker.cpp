// embree_picker.cpp
#include "embree_picker.hpp"
#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <ranges>
#include <limits>
#include <glm/gtc/type_ptr.hpp>

#include "system/pick_system.hpp"
#include <pmmintrin.h>
#include <tracy/Tracy.hpp>
namespace {
void warmup_embree(RTCScene scene) {
    RTCRayHit rayhit{};
    rayhit.ray.org_x = 0.0f;
    rayhit.ray.org_y = 0.0f;
    rayhit.ray.org_z = 0.0f;
    rayhit.ray.dir_x = 0.0f;
    rayhit.ray.dir_y = 0.0f;
    rayhit.ray.dir_z = 1.0f;
    rayhit.ray.tnear = 0.0f;
    rayhit.ray.tfar = std::numeric_limits<float>::infinity();
    rayhit.ray.mask = std::numeric_limits<unsigned int>::max();
    ;
    rayhit.ray.time = 0.0f;
    rayhit.hit.geomID = RTC_INVALID_GEOMETRY_ID;

    rtcIntersect1(scene, &rayhit);  // 触发 JIT + BVH build
}
}  // namespace
namespace graphics {

EmbreePicker::EmbreePicker() : device_(rtcNewDevice(nullptr)), scene_(nullptr) {
    if (!device_) {
        throw std::runtime_error("Failed to create Embree device");
    }

    // 全局设置浮点行为
    _MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
    _MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);

    // 创建场景
    scene_ = rtcNewScene(device_);
    rtcSetSceneFlags(scene_, RTC_SCENE_FLAG_ROBUST);
    rtcSetSceneBuildQuality(scene_, RTC_BUILD_QUALITY_HIGH);

    // 创建主动态场景（用于 instances）
    main_scene_ = rtcNewScene(device_);
    rtcSetSceneFlags(main_scene_, RTC_SCENE_FLAG_DYNAMIC | RTC_SCENE_FLAG_ROBUST);
    rtcSetSceneBuildQuality(main_scene_, RTC_BUILD_QUALITY_LOW);
}

EmbreePicker::~EmbreePicker() {
    // 直接遍历 values
    for (const auto& value : geometries_ | std::views::values) {
        if (value) {
            rtcReleaseGeometry(value);
        }
    }
    if (scene_) {
        rtcReleaseScene(scene_);
    }
    if (device_) {
        rtcReleaseDevice(device_);
    }
}

void EmbreePicker::buildMesh(id_t id, std::span<const glm::vec3> vertices,
                             std::span<const uint32_t> indices, bool rebuild) {
    if (geometries_.contains(id)) {
        ZoneScopedN("EmbreePicker::buildMesh");
        if (!rebuild) {
            return;
        }
        rtcReleaseGeometry(geometries_.find(id)->second);
        geometries_.erase(id);

        // 释放实例（如果存在）
        if (instances_.contains(id)) {
            for (auto& [k, v] : embree_to_user) {
                if (v == id) {
                    rtcDetachGeometry(main_scene_, k);
                    embree_to_user.erase(k);
                    break;
                }
            }
            rtcReleaseGeometry(instances_[id]);
            instances_.erase(id);
        }
    }

    // 2. 创建三角形网格几何体
    RTCGeometry geometry_ = rtcNewGeometry(device_, RTC_GEOMETRY_TYPE_TRIANGLE);

    // 3. 设置顶点
    auto* vertices_mapped = static_cast<glm::vec3*>(
        rtcSetNewGeometryBuffer(geometry_, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3,
                                sizeof(glm::vec3), vertices.size()));
    std::ranges::copy(vertices, vertices_mapped);

    // 4. 设置索引
    auto* indices_mapped = static_cast<uint32_t*>(
        rtcSetNewGeometryBuffer(geometry_, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3,
                                sizeof(uint32_t) * 3, indices.size() / 3));
    std::ranges::copy(indices, indices_mapped);

    // 5. 提交几何体（构建 BVH）
    rtcCommitGeometry(geometry_);

    geometries_[id] = geometry_;

    // 4. 创建 instance（在主场景中）
    RTCGeometry instance = rtcNewGeometry(device_, RTC_GEOMETRY_TYPE_INSTANCE);

    // 设置 instance 引用共享场景
    rtcSetGeometryInstancedScene(instance, scene_);

    // 初始 transform
    glm::mat4 identity(1.0f);
    rtcSetGeometryTransform(instance, 0, RTC_FORMAT_FLOAT4X4_COLUMN_MAJOR,
                            glm::value_ptr(identity));

    rtcCommitGeometry(instance);

    // 5. 添加 instance 到主场景
    unsigned int instance_id = rtcAttachGeometry(main_scene_, instance);
    // 6. 将几何体添加到场景
    rtcAttachGeometry(scene_, geometry_);
    instances_[id] = instance;

    // 6. 映射拾取 ID
    embree_to_user[instance_id] = id;
    commit();
    warmup_embree(main_scene_);
}

// 在每帧更新所有移动物体的 transform
void EmbreePicker::updateTransform(id_t id, const ecs::TransformComponent& transform) {
    if (!geometries_.contains(id)) {
        return;
    }
    auto* geometry = instances_.find(id)->second;

    rtcSetGeometryTransform(geometry, 0, RTC_FORMAT_FLOAT4X4_COLUMN_MAJOR,
                            glm::value_ptr(transform.mat4()));
    rtcCommitGeometry(geometry);
}
void EmbreePicker::commit() {
    ZoneScopedN("EmbreePicker::commit");
    rtcCommitScene(scene_);
    rtcCommitScene(main_scene_);
}

auto EmbreePicker::pick(const glm::vec3& rayOrigin, const glm::vec3& rayDirection)
    -> std::optional<PickResult> {
    ZoneScopedN("EmbreePicker::pick");

    RTCRayHit ray_hit{};
    ray_hit.ray.org_x = rayOrigin.x;
    ray_hit.ray.org_y = rayOrigin.y;
    ray_hit.ray.org_z = rayOrigin.z;

    ray_hit.ray.dir_x = rayDirection.x;
    ray_hit.ray.dir_y = rayDirection.y;
    ray_hit.ray.dir_z = rayDirection.z;

    ray_hit.ray.tnear = 1e-4f;  // 避免自相交
    ray_hit.ray.tfar = std::numeric_limits<float>::max();
    ray_hit.ray.mask = std::numeric_limits<unsigned int>::max();
    ;
    ray_hit.ray.time = 0.f;

    ray_hit.hit.geomID = RTC_INVALID_GEOMETRY_ID;
    ray_hit.hit.primID = RTC_INVALID_GEOMETRY_ID;
    ray_hit.hit.instID[0] = RTC_INVALID_GEOMETRY_ID;

    // 执行单条射线相交
    rtcIntersect1(main_scene_, &ray_hit);

    if (ray_hit.hit.primID != RTC_INVALID_GEOMETRY_ID) {
        return PickResult{.position = rayOrigin + rayDirection * ray_hit.ray.tfar,
                          .distance = ray_hit.ray.tfar,
                          .primitiveId = ray_hit.hit.primID,
                          .id = embree_to_user.at(ray_hit.hit.geomID)};
    }

    return std::nullopt;
}

}  // namespace graphics
