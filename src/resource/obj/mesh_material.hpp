#pragma once
#include <assimp/scene.h>
#include <glm/glm.hpp>
#include <string>
#include <ostream>
namespace graphics {
struct MeshMaterial {
        std::string name = "default";

        // 颜色属性（来自 .mtl）
        glm::vec3 ambientColor = {1.0f, 1.0f, 1.0f};   // Ka
        glm::vec3 diffuseColor = {1.0f, 1.0f, 1.0f};   // Kd
        glm::vec3 specularColor = {.04f, .04f, .04f};  // Ks
        glm::vec3 emissiveColor = {0.0f, 0.0f, 0.0f};  // Ke

        // 标量属性
        float shininess = 32.0f;  // Ns
        float opacity = 1.0f;     // d
        float ior = 1.0f;         // Ni

        // PBR
        float metallic = 0.0f;
        float roughness = .5f;
        float ao = 1.0f;

        // 纹理路径 —— 改为 vector 支持多贴图
        std::vector<std::string> ambientTextures;   // map_Ka
        std::vector<std::string> diffuseTextures;   // map_Kd
        std::vector<std::string> specularTextures;  // map_Ks
        std::vector<std::string> normalTextures;    // map_Bump / map_Ns
        std::vector<std::string> emissiveTextures;  // map_Ke
        std::vector<std::string> aoTextures;
        std::vector<std::string> metallicTextures;
        std::vector<std::string> albedoTextures;
        std::vector<std::string> metallicRoughnessTextures;
        std::vector<std::string> heightTextures;

        // ----------------------------
        // 序列化：写入二进制流
        // ----------------------------
        void serialize(std::ostream& os) const;
        // ----------------------------
        // 反序列化：从二进制流读取
        // ----------------------------
        [[nodiscard]] auto deserialize(std::istream& is) -> bool;
};

auto loadMaterial(const aiScene* scene, const aiMesh* mesh) -> MeshMaterial;
}  // namespace graphics