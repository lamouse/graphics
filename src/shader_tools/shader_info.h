#pragma once

#include <cstdint>
#include "shader_enums.hpp"
#include "render_core/vertex.hpp"

#include <boost/container/small_vector.hpp>
#include <boost/container/static_vector.hpp>
namespace shader {

constexpr uint8_t NUM_TEXTURE_TYPES = 9;

struct UniformBufferDescriptor {
        uint32_t binding;
        uint32_t count;
        uint32_t set;
        auto operator<=>(const UniformBufferDescriptor&) const = default;
};
struct StorageBufferDescriptor {
        uint32_t binding;
        uint32_t count;
        uint32_t set;
        bool is_written;

        auto operator<=>(const StorageBufferDescriptor&) const = default;
};
struct TextureBufferDescriptor {
        uint32_t binding;
        uint32_t count;
        TexturePixelFormat format;
        auto operator<=>(const TextureBufferDescriptor&) const = default;
};
using TextureBufferDescriptors = boost::container::small_vector<TextureBufferDescriptor, 6>;
struct ImageBufferDescriptor {
        ImageFormat format;
        bool is_written;
        bool is_read;
        bool is_integer;
        uint32_t binding;
        uint32_t count;
        uint32_t set;

        auto operator<=>(const ImageBufferDescriptor&) const = default;
};
using ImageBufferDescriptors = boost::container::small_vector<ImageBufferDescriptor, 2>;
struct TextureDescriptor {
        TextureType type;
        uint32_t binding;
        uint32_t count;
        uint32_t set;
        bool is_depth;
        bool is_multisample;

        auto operator<=>(const TextureDescriptor&) const = default;
};
using TextureDescriptors = boost::container::small_vector<TextureDescriptor, 12>;

struct ImageDescriptor {
        TextureType type;
        ImageFormat format;
        bool is_written;
        bool is_read;
        bool is_integer;
        uint32_t binding;
        uint32_t count;
        uint32_t set;
        uint32_t size_shift;
        auto operator<=>(const ImageDescriptor&) const = default;
};
using ImageDescriptors = boost::container::small_vector<ImageDescriptor, 4>;

struct Info {
        static constexpr size_t MAX_CBUFS{18};
        static constexpr size_t MAX_SSBOS{32};

        boost::container::static_vector<UniformBufferDescriptor, MAX_CBUFS>
            uniform_buffer_descriptors;
        boost::container::static_vector<StorageBufferDescriptor, MAX_SSBOS>
            storage_buffers_descriptors;
        boost::container::static_vector<render::VertexBinding, 32> vertexBindings;
        boost::container::static_vector<render::VertexAttribute, 32> vertexAttribute;
        TextureBufferDescriptors texture_buffer_descriptors;
        ImageBufferDescriptors image_buffer_descriptors;
        TextureDescriptors texture_descriptors;
        ImageDescriptors image_descriptors;
};
template <typename Descriptors>
auto NumDescriptors(const Descriptors& descriptors) -> uint32_t {
    uint32_t num{};
    for (const auto& desc : descriptors) {
        num += desc.count;
    }
    return num;
}

}  // namespace shader