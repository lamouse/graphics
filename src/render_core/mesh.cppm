module;
#include "render_core/mesh.hpp"
#include "render_core/vertex.hpp"
export module render.resource.mesh;
export namespace render{
    using IMeshData = render::IMeshData;
    using IMeshInstance = render::IMeshInstance;
    using ITexture = render::ITexture;
    using VertexBinding = render::VertexBinding;
    using VertexAttribute = render::VertexAttribute;
}