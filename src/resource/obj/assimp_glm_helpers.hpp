#pragma once
#include <assimp/quaternion.h>
#include <assimp/vector3.h>
#include <assimp/matrix4x4.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
namespace graphics {

class AssimpGLMHelpers {
    public:
        static inline auto convert(const aiMatrix4x4& from) -> glm::mat4 {
            glm::mat4 to;
            // the a,b,c,d in assimp is the row ; the 1,2,3,4 is the column
            // clang-format off
            to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4; // from a
            to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4; // from b
            to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4; // from c
            to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4; // from d
            // clang-format on
            return to;
        }
        static inline auto convert(const aiVector3D& vec) -> glm::vec3 {
            return glm::vec3(vec.x, vec.y, vec.z);
        }

        static inline auto convert(const aiQuaternion& quaternion) -> glm::quat {
            return glm::quat(quaternion.w, quaternion.x, quaternion.y, quaternion.z);
        }
};

}  // namespace graphics::animation