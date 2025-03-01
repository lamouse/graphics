#pragma once

#include <compare>
#include <cstdint>
#include "shader_enums.hpp"
#include <boost/container/small_vector.hpp>
#include <boost/container/static_vector.hpp>
namespace shader {

constexpr uint8_t NUM_TEXTURE_TYPES = 9;

struct ConstantBufferDescriptor {
        uint32_t index;
        uint32_t count;
        auto operator<=>(const ConstantBufferDescriptor&) const = default;
};
struct StorageBufferDescriptor {
        uint32_t cbuf_index;
        uint32_t cbuf_offset;
        uint32_t count;
        bool is_written;

        auto operator<=>(const StorageBufferDescriptor&) const = default;
};
struct TextureBufferDescriptor {
        bool has_secondary;
        uint32_t cbuf_index;
        uint32_t cbuf_offset;
        uint32_t shift_left;
        uint32_t secondary_cbuf_index;
        uint32_t secondary_cbuf_offset;
        uint32_t secondary_shift_left;
        uint32_t count;
        uint32_t size_shift;

        auto operator<=>(const TextureBufferDescriptor&) const = default;
};
using TextureBufferDescriptors = boost::container::small_vector<TextureBufferDescriptor, 6>;
struct ImageBufferDescriptor {
        ImageFormat format;
        bool is_written;
        bool is_read;
        bool is_integer;
        uint32_t cbuf_index;
        uint32_t cbuf_offset;
        uint32_t count;
        uint32_t size_shift;

        auto operator<=>(const ImageBufferDescriptor&) const = default;
};
using ImageBufferDescriptors = boost::container::small_vector<ImageBufferDescriptor, 2>;
struct TextureDescriptor {
        TextureType type;
        bool is_depth;
        bool is_multisample;
        bool has_secondary;
        uint32_t cbuf_index;
        uint32_t cbuf_offset;
        uint32_t shift_left;
        uint32_t secondary_cbuf_index;
        uint32_t secondary_cbuf_offset;
        uint32_t secondary_shift_left;
        uint32_t count;
        uint32_t size_shift;

        auto operator<=>(const TextureDescriptor&) const = default;
};
using TextureDescriptors = boost::container::small_vector<TextureDescriptor, 12>;

struct ImageDescriptor {
        TextureType type;
        ImageFormat format;
        bool is_written;
        bool is_read;
        bool is_integer;
        uint32_t count;
        uint32_t size_shift;
        auto operator<=>(const ImageDescriptor&) const = default;
};
using ImageDescriptors = boost::container::small_vector<ImageDescriptor, 4>;

struct Info {
        static constexpr size_t MAX_CBUFS{18};
        static constexpr size_t MAX_SSBOS{32};

        boost::container::static_vector<ConstantBufferDescriptor, MAX_CBUFS>
            constant_buffer_descriptors;
        boost::container::static_vector<StorageBufferDescriptor, MAX_SSBOS>
            storage_buffers_descriptors;
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