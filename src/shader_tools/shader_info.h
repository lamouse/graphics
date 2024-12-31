#pragma once

#include <compare>
#include <cstdint>
#include <array>
#include <bitset>
#include <map>
#include "shader_enums.hpp"
#include "fronted/ir/type.hpp"
#include "varying_state.hpp"
#include "fronted/ir/attribute.hpp"
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
        uint32_t cbuf_index;
        uint32_t cbuf_offset;
        uint32_t count;
        uint32_t size_shift;

        auto operator<=>(const ImageDescriptor&) const = default;
};
using ImageDescriptors = boost::container::small_vector<ImageDescriptor, 4>;

struct Info {
        static constexpr size_t MAX_INDIRECT_CBUFS{14};
        static constexpr size_t MAX_CBUFS{18};
        static constexpr size_t MAX_SSBOS{32};

        bool uses_workgroup_id{};
        bool uses_local_invocation_id{};
        bool uses_invocation_id{};
        bool uses_invocation_info{};
        bool uses_sample_id{};
        bool uses_is_helper_invocation{};
        bool uses_subgroup_invocation_id{};
        bool uses_subgroup_shuffles{};
        std::array<bool, 30> uses_patches{};

        std::array<Interpolation, 32> interpolation{};
        VaryingState loads;
        VaryingState stores;
        VaryingState passthrough;

        std::map<IR::Attribute, IR::Attribute> legacy_stores_mapping;

        bool loads_indexed_attributes{};

        std::array<bool, 8> stores_frag_color{};
        bool stores_sample_mask{};
        bool stores_frag_depth{};

        bool stores_tess_level_outer{};
        bool stores_tess_level_inner{};

        bool stores_indexed_attributes{};

        bool stores_global_memory{};
        bool uses_local_memory{};

        bool uses_fp16{};
        bool uses_fp64{};
        bool uses_fp16_denorms_flush{};
        bool uses_fp16_denorms_preserve{};
        bool uses_fp32_denorms_flush{};
        bool uses_fp32_denorms_preserve{};
        bool uses_int8{};
        bool uses_int16{};
        bool uses_int64{};
        bool uses_image_1d{};
        bool uses_sampled_1d{};
        bool uses_sparse_residency{};
        bool uses_demote_to_helper_invocation{};
        bool uses_subgroup_vote{};
        bool uses_subgroup_mask{};
        bool uses_fswzadd{};
        bool uses_derivatives{};
        bool uses_typeless_image_reads{};
        bool uses_typeless_image_writes{};
        bool uses_image_buffers{};
        bool uses_shared_increment{};
        bool uses_shared_decrement{};
        bool uses_global_increment{};
        bool uses_global_decrement{};
        bool uses_atomic_f32_add{};
        bool uses_atomic_f16x2_add{};
        bool uses_atomic_f16x2_min{};
        bool uses_atomic_f16x2_max{};
        bool uses_atomic_f32x2_add{};
        bool uses_atomic_f32x2_min{};
        bool uses_atomic_f32x2_max{};
        bool uses_atomic_s32_min{};
        bool uses_atomic_s32_max{};
        bool uses_int64_bit_atomics{};
        bool uses_global_memory{};
        bool uses_atomic_image_u32{};
        bool uses_shadow_lod{};
        bool uses_rescaling_uniform{};
        bool uses_cbuf_indirect{};
        bool uses_render_area{};

        IR::Type used_constant_buffer_types{};
        IR::Type used_storage_buffer_types{};
        IR::Type used_indirect_cbuf_types{};

        uint32_t constant_buffer_mask{};
        std::array<uint32_t, MAX_CBUFS> constant_buffer_used_sizes{};
        uint32_t nvn_buffer_base{};
        std::bitset<16> nvn_buffer_used{};

        bool requires_layer_emulation{};
        IR::Attribute emulated_layer{};

        uint32_t used_clip_distances{};

        boost::container::static_vector<ConstantBufferDescriptor, MAX_CBUFS> constant_buffer_descriptors;
        boost::container::static_vector<StorageBufferDescriptor, MAX_SSBOS> storage_buffers_descriptors;
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