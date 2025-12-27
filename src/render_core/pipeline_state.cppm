module;
#include "render_core/pipeline_state.h"
export module render.pipeline_state;
export namespace render{
using PrimitiveTopology = render::PrimitiveTopology;
using MsaaMode = render::MsaaMode;
using DynamicFeatures = render::DynamicFeatures;
using FixedPipelineState = render::FixedPipelineState;
using IndexFormat = render::IndexFormat;
using DynamicPipelineState = render::DynamicPipelineState;
using ComparisonOp = render::ComparisonOp;
using StencilOp = render::StencilOp;
using CullFace = render::CullFace;
}