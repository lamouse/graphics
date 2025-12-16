#include "resource/obj/mesh_material.hpp"
#include "resource/obj/mesh_util.hpp"

namespace {
auto getTexturePath(aiMaterial* mat, aiTextureType type) -> std::vector<std::string> {
    std::vector<std::string> paths;
    for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
        aiString str;
        if (mat->GetTexture(type, i, &str) == AI_SUCCESS) {
            paths.emplace_back(str.C_Str());
        }
    }
    return paths;
}
}  // namespace

namespace graphics {
void MeshMaterial::serialize(std::ostream& os) const {
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

auto MeshMaterial::deserialize(std::istream& is) -> bool {
    // 读取颜色
    if (!is.read(reinterpret_cast<char*>(&ambientColor), sizeof(ambientColor))) {
        return false;
    }
    if (!is.read(reinterpret_cast<char*>(&diffuseColor), sizeof(diffuseColor))) {
        return false;
    }
    if (!is.read(reinterpret_cast<char*>(&specularColor), sizeof(specularColor))) {
        return false;
    }
    if (!is.read(reinterpret_cast<char*>(&emissiveColor), sizeof(emissiveColor))) {
        return false;
    }

    // 读取标量
    if (!is.read(reinterpret_cast<char*>(&shininess), sizeof(shininess))) {
        return false;
    }
    if (!is.read(reinterpret_cast<char*>(&opacity), sizeof(opacity))) {
        return false;
    }
    if (!is.read(reinterpret_cast<char*>(&ior), sizeof(ior))) {
        return false;
    }
    if (!is.read(reinterpret_cast<char*>(&metallic), sizeof(metallic))) {
        return false;
    }
    if (!is.read(reinterpret_cast<char*>(&roughness), sizeof(roughness))) {
        return false;
    }
    if (!is.read(reinterpret_cast<char*>(&ao), sizeof(ao))) {
        return false;
    }

    // 读取 name
    {
        uint32_t len = 0;
        if (!is.read(reinterpret_cast<char*>(&len), sizeof(len))) {
            return false;
        }
        if (len > 0) {
            name.resize(len);
            if (!is.read(name.data(), len)) {
                return false;
            }
        } else {
            name.clear();
        }
    }

    // 读取所有纹理 vector
    if (!read_string_vector(is, ambientTextures)) {
        return false;
    }
    if (!read_string_vector(is, diffuseTextures)) {
        return false;
    }
    if (!read_string_vector(is, specularTextures)) {
        return false;
    }
    if (!read_string_vector(is, normalTextures)) {
        return false;
    }
    if (!read_string_vector(is, emissiveTextures)) {
        return false;
    }
    if (!read_string_vector(is, aoTextures)) {
        return false;
    }
    if (!read_string_vector(is, metallicTextures)) {
        return false;
    }
    if (!read_string_vector(is, albedoTextures)) {
        return false;
    }
    if (!read_string_vector(is, metallicRoughnessTextures)) {
        return false;
    }
    if (!read_string_vector(is, heightTextures)) {
        return false;
    }

    return true;
}
auto loadMaterial(const aiScene* scene, const aiMesh* mesh) -> MeshMaterial {
    graphics::MeshMaterial mat{};
    if (scene->HasMaterials() && mesh->mMaterialIndex < scene->mNumMaterials) {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        // 名称
        aiString name;
        if (material->Get(AI_MATKEY_NAME, name) == AI_SUCCESS) {
            mat.name = name.C_Str();
        }

        // 颜色
        aiColor3D ka(1.f, 1.f, 1.f), kd(1.f, 1.f, 1.f), ks(1.f, 1.f, 1.f), ke(0.f, 0.f, 0.f);
        if (material->Get(AI_MATKEY_COLOR_AMBIENT, ka) == AI_SUCCESS && !ka.IsBlack()) {
            mat.ambientColor = {ka.r, ka.g, ka.b};
        };

        if (material->Get(AI_MATKEY_COLOR_DIFFUSE, kd) == AI_SUCCESS && !kd.IsBlack()) {
            mat.diffuseColor = {kd.r, kd.g, kd.b};
        }
        if (material->Get(AI_MATKEY_COLOR_SPECULAR, ks) == AI_SUCCESS && !ks.IsBlack()) {
            mat.specularColor = {ks.r, ks.g, ks.b};
        }

        if (material->Get(AI_MATKEY_COLOR_EMISSIVE, ke) == AI_SUCCESS && !ke.IsBlack()) {
            mat.emissiveColor = {ke.r, ke.g, ke.b};
        }
        // 标量
        ai_real shininess = NAN;
        if (material->Get(AI_MATKEY_SHININESS, shininess) == AI_SUCCESS && !std::isnan(shininess)) {
            if (shininess > .0f) {
                mat.shininess = shininess;
            }
        }

        ai_real opacity = NAN;
        if (material->Get(AI_MATKEY_OPACITY, opacity) == AI_SUCCESS && !std::isnan(opacity)) {
            mat.opacity = opacity;
        }
        ai_real ior = NAN;
        if (material->Get(AI_MATKEY_REFRACTI, ior) == AI_SUCCESS && !std::isnan(ior)) {
            mat.ior = ior;
        }

        ai_real value = NAN;
        if (material->Get(AI_MATKEY_METALLIC_FACTOR, value) == AI_SUCCESS && !std::isnan(value)) {
            mat.metallic = static_cast<float>(value);
        }

        ai_real roughness_value = NAN;
        if (material->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness_value) == AI_SUCCESS &&
            !std::isnan(roughness_value)) {
            mat.roughness = static_cast<float>(roughness_value);
        }

        // 纹理
        mat.ambientTextures = getTexturePath(material, aiTextureType_AMBIENT);
        mat.diffuseTextures = getTexturePath(material, aiTextureType_DIFFUSE);
        mat.specularTextures = getTexturePath(material, aiTextureType_SPECULAR);
        mat.heightTextures = getTexturePath(material, aiTextureType_HEIGHT);
        mat.normalTextures = getTexturePath(material, aiTextureType_NORMALS);
        if (mat.normalTextures.empty()) {
            mat.normalTextures = mat.heightTextures;
        }
        mat.emissiveTextures = getTexturePath(material, aiTextureType_EMISSIVE);

        // ========== PBR 贴图（Assimp 5.0+ 支持）==========

        // Albedo (Base Color) —— PBR 中的 diffuse 替代
        mat.albedoTextures = getTexturePath(material, aiTextureType_BASE_COLOR);
        if (mat.albedoTextures.empty()) {
            mat.albedoTextures = mat.diffuseTextures;
        }
        // Metallic
        mat.metallicTextures = getTexturePath(material, aiTextureType_METALNESS);
        // Roughness
        mat.metallicRoughnessTextures = getTexturePath(material, aiTextureType_DIFFUSE_ROUGHNESS);

        mat.aoTextures = getTexturePath(material, aiTextureType_AMBIENT_OCCLUSION);
        if (mat.aoTextures.empty()) {
            mat.aoTextures = mat.ambientTextures;
        }
    }
    return mat;
}

}  // namespace graphics