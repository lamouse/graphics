#include "resource/obj/mesh_material.hpp"
#include "resource/obj/mesh_util.hpp"

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
}  // namespace graphics