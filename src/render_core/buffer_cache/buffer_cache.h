#pragma once

#include "render_core/buffer_cache/buffer_cache_base.hpp"
namespace render::buffer {
template <class P>
BufferCache<P>::BufferCache(Runtime& runtime_) : runtime{runtime_} {}
}  // namespace render::buffer