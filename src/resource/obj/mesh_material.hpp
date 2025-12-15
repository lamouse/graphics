#pragma once
#include <glm/glm.hpp>
#include <string>
#include <ostream>
#include "resource/obj/mesh_util.hpp"
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
        void serialize(std::ostream& os) const {
            // 写入颜色
            os.write(reinterpret_cast<const char*>(&ambientColor), sizeof(ambientColor));
            os.write(reinterpret_cast<const char*>(&diffuseColor), sizeof(diffuseColor));
            os.write(reinterpret_cast<const char*>(&specularColor), sizeof(specularColor));
            os.write(reinterpret_cast<const char*>(&emissiveColor), sizeof(emissiveColor));

            // 写入标量
            os.write(reinterpret_cast<const char*>(&shininess), sizeof(shininess));
            os.write(reinterpret_cast<const char*>(&opacity), sizeof(opacity));
            os.write(reinterpret_cast<const char*>(&ior), sizeof(ior));

            // 写入PBR
            os.write(reinterpret_cast<const char*>(&metallic), sizeof(metallic));
            os.write(reinterpret_cast<const char*>(&roughness), sizeof(roughness));
            os.write(reinterpret_cast<const char*>(&ao), sizeof(ao));

            // 写入 name（保持为单 string）
            {
                auto len = static_cast<uint32_t>(name.size());
                os.write(reinterpret_cast<const char*>(&len), sizeof(len));
                if (len > 0) os.write(name.data(), len);
            }

            // 写入所有纹理 vector
            write_string_vector(os, ambientTextures);
            write_string_vector(os, diffuseTextures);
            write_string_vector(os, specularTextures);
            write_string_vector(os, normalTextures);
            write_string_vector(os, emissiveTextures);
            write_string_vector(os, aoTextures);
            write_string_vector(os, metallicTextures);
            write_string_vector(os, albedoTextures);
            write_string_vector(os, metallicRoughnessTextures);
            write_string_vector(os, heightTextures);
        }

        // ----------------------------
        // 反序列化：从二进制流读取
        // ----------------------------
        [[nodiscard]] auto deserialize(std::istream& is) -> bool {
            // 读取颜色
            if (!is.read(reinterpret_cast<char*>(&ambientColor), sizeof(ambientColor)))
                return false;
            if (!is.read(reinterpret_cast<char*>(&diffuseColor), sizeof(diffuseColor)))
                return false;
            if (!is.read(reinterpret_cast<char*>(&specularColor), sizeof(specularColor)))
                return false;
            if (!is.read(reinterpret_cast<char*>(&emissiveColor), sizeof(emissiveColor)))
                return false;

            // 读取标量
            if (!is.read(reinterpret_cast<char*>(&shininess), sizeof(shininess))) return false;
            if (!is.read(reinterpret_cast<char*>(&opacity), sizeof(opacity))) return false;
            if (!is.read(reinterpret_cast<char*>(&ior), sizeof(ior))) return false;
            if (!is.read(reinterpret_cast<char*>(&metallic), sizeof(metallic))) return false;
            if (!is.read(reinterpret_cast<char*>(&roughness), sizeof(roughness))) return false;
            if (!is.read(reinterpret_cast<char*>(&ao), sizeof(ao))) return false;

            // 读取 name
            {
                uint32_t len = 0;
                if (!is.read(reinterpret_cast<char*>(&len), sizeof(len))) return false;
                if (len > 0) {
                    name.resize(len);
                    if (!is.read(name.data(), len)) return false;
                } else {
                    name.clear();
                }
            }

            // 读取所有纹理 vector
            if (!read_string_vector(is, ambientTextures)) return false;
            if (!read_string_vector(is, diffuseTextures)) return false;
            if (!read_string_vector(is, specularTextures)) return false;
            if (!read_string_vector(is, normalTextures)) return false;
            if (!read_string_vector(is, emissiveTextures)) return false;
            if (!read_string_vector(is, aoTextures)) return false;
            if (!read_string_vector(is, metallicTextures)) return false;
            if (!read_string_vector(is, albedoTextures)) return false;
            if (!read_string_vector(is, metallicRoughnessTextures)) return false;
            if (!read_string_vector(is, heightTextures)) return false;

            return true;
        }
};
}