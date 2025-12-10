#include "render_core/shader_cache.hpp"
#include "common/assert.hpp"
#include <xxhash.h>
namespace render {
auto ShaderCache::addShader(std::span<const u32> data, ShaderType type) -> u64 {
    ASSERT_MSG(!data.empty(), "upload empty shader data");
    auto info = std::make_unique<ShaderInfo>();
    XXH64_hash_t hash = XXH64(data.data(), data.size() * sizeof(u32), 0);
    info->type = type;
    info->unique_hash = hash;
    info->loaded = false;
    load_data.emplace(hash, std::vector<u32>(std::from_range, data));
    storage[hash] = std::move(info);
    return hash;
}

void ShaderCache::setCurrentShader(u64 vertexHash, u64 fragmentHas) {
    if (shader_infos[static_cast<u8>(ShaderType::Vertex)] &&
        shader_infos[static_cast<u8>(ShaderType::Vertex)]->unique_hash == vertexHash &&
        shader_infos[static_cast<u8>(ShaderType::Fragment)] &&
        shader_infos[static_cast<u8>(ShaderType::Fragment)]->unique_hash == fragmentHas) {
        return;
    }
    ASSERT_MSG(storage.contains(vertexHash) && storage.contains(fragmentHas),
               "vertex or fragment shader not found");

    shader_infos[static_cast<u8>(ShaderType::Vertex)] = storage.find(vertexHash)->second.get();
    shader_infos[static_cast<u8>(ShaderType::Fragment)] = storage.find(fragmentHas)->second.get();
    shader_infos[static_cast<u8>(ShaderType::Compute)] = nullptr;
}
void ShaderCache::setsetCurrentShader(u64 computeHash) {
    if (shader_infos[static_cast<u8>(ShaderType::Compute)] &&
        shader_infos[static_cast<u8>(ShaderType::Compute)]->unique_hash == computeHash) {
        return;
    }
    ASSERT_MSG(storage.contains(computeHash), "compute shader not found");

    shader_infos[static_cast<u8>(ShaderType::Vertex)] = nullptr;
    shader_infos[static_cast<u8>(ShaderType::Fragment)] = nullptr;
    shader_infos[static_cast<u8>(ShaderType::Compute)] = storage.find(computeHash)->second.get();
}

void ShaderCache::mark_loaded(const std::array<const ShaderInfo*, NUM_PROGRAMS>& infos) {
    for (const auto* info : infos) {
        if (!info) {
            continue;
        }
        if (info->loaded) {
            continue;
        }
        storage.find(info->unique_hash)->second->loaded = true;
        load_data.erase(info->unique_hash);
    }
}
auto ShaderCache::getShaderData(u64 hash) -> std::span<const u32> {
    return load_data.find(hash)->second;
}
}  // namespace render