#include "shader_compile.hpp"
#include <filesystem>
#include <fstream>
#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>
#include <spirv_cross/spirv_cross.hpp>
#include <spirv_cross/spirv_glsl.hpp>
#include <spdlog/spdlog.h>
namespace {
namespace fs = std::filesystem;

auto list_files(const fs::path& directory) -> std::vector<fs::path> {
    std::vector<fs::path> file_list;
    if (!fs::exists(directory)) {
        throw std::runtime_error("Directory does not exist: " + directory.string());
    }
    for (const auto& entry : fs::recursive_directory_iterator(directory)) {
        if (fs::is_regular_file(entry)) {
            file_list.push_back(entry.path());
        }
    }
    return file_list;
}
auto read_shader(const fs::path& path) -> std::string {
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    auto file_size = file.tellg();
    std::string buffer(file_size, ' ');
    file.seekg(0);
    file.read(buffer.data(), file_size);
    return buffer;
}
auto getDefaultTBuiltInResource() -> TBuiltInResource {
    TBuiltInResource resource = {};
    resource.maxLights = 32;
    resource.maxClipPlanes = 6;
    resource.maxTextureUnits = 32;
    resource.maxTextureCoords = 32;
    resource.maxVertexAttribs = 64;
    resource.maxVertexUniformComponents = 4096;
    resource.maxVaryingFloats = 64;
    resource.maxVertexTextureImageUnits = 32;
    resource.maxCombinedTextureImageUnits = 80;
    resource.maxTextureImageUnits = 32;
    resource.maxFragmentUniformComponents = 4096;
    resource.maxDrawBuffers = 32;
    resource.maxVertexUniformVectors = 128;
    resource.maxVaryingVectors = 8;
    resource.maxFragmentUniformVectors = 16;
    resource.maxVertexOutputVectors = 16;
    resource.maxFragmentInputVectors = 15;
    resource.minProgramTexelOffset = -8;
    resource.maxProgramTexelOffset = 7;
    resource.maxClipDistances = 8;
    resource.maxComputeWorkGroupCountX = 65535;
    resource.maxComputeWorkGroupCountY = 65535;
    resource.maxComputeWorkGroupCountZ = 65535;
    resource.maxComputeWorkGroupSizeX = 1024;
    resource.maxComputeWorkGroupSizeY = 1024;
    resource.maxComputeWorkGroupSizeZ = 64;
    resource.maxComputeUniformComponents = 1024;
    resource.maxComputeTextureImageUnits = 16;
    resource.maxComputeImageUniforms = 8;
    resource.maxComputeAtomicCounters = 8;
    resource.maxComputeAtomicCounterBuffers = 1;
    resource.maxVaryingComponents = 60;
    resource.maxVertexOutputComponents = 64;
    resource.maxGeometryInputComponents = 64;
    resource.maxGeometryOutputComponents = 128;
    resource.maxFragmentInputComponents = 128;
    resource.maxImageUnits = 8;
    resource.maxCombinedImageUnitsAndFragmentOutputs = 8;
    resource.maxCombinedShaderOutputResources = 8;
    resource.maxImageSamples = 0;
    resource.maxVertexImageUniforms = 0;
    resource.maxTessControlImageUniforms = 0;
    resource.maxTessEvaluationImageUniforms = 0;
    resource.maxGeometryImageUniforms = 0;
    resource.maxFragmentImageUniforms = 8;
    resource.maxCombinedImageUniforms = 8;
    resource.maxGeometryTextureImageUnits = 16;
    resource.maxGeometryOutputVertices = 256;
    resource.maxGeometryTotalOutputComponents = 1024;
    resource.maxGeometryUniformComponents = 1024;
    resource.maxGeometryVaryingComponents = 64;
    resource.maxTessControlInputComponents = 128;
    resource.maxTessControlOutputComponents = 128;
    resource.maxTessControlTextureImageUnits = 16;
    resource.maxTessControlUniformComponents = 1024;
    resource.maxTessControlTotalOutputComponents = 4096;
    resource.maxTessEvaluationInputComponents = 128;
    resource.maxTessEvaluationOutputComponents = 128;
    resource.maxTessEvaluationTextureImageUnits = 16;
    resource.maxTessEvaluationUniformComponents = 1024;
    resource.maxTessPatchComponents = 120;
    resource.maxPatchVertices = 32;
    resource.maxTessGenLevel = 64;
    resource.maxViewports = 16;
    resource.maxVertexAtomicCounters = 0;
    resource.maxTessControlAtomicCounters = 0;
    resource.maxTessEvaluationAtomicCounters = 0;
    resource.maxGeometryAtomicCounters = 0;
    resource.maxFragmentAtomicCounters = 8;
    resource.maxCombinedAtomicCounters = 8;
    resource.maxAtomicCounterBindings = 1;
    resource.maxVertexAtomicCounterBuffers = 0;
    resource.maxTessControlAtomicCounterBuffers = 0;
    resource.maxTessEvaluationAtomicCounterBuffers = 0;
    resource.maxGeometryAtomicCounterBuffers = 0;
    resource.maxFragmentAtomicCounterBuffers = 1;
    resource.maxCombinedAtomicCounterBuffers = 1;
    resource.maxAtomicCounterBufferSize = 16384;
    resource.maxTransformFeedbackBuffers = 4;
    resource.maxTransformFeedbackInterleavedComponents = 64;
    resource.maxCullDistances = 8;
    resource.maxCombinedClipAndCullDistances = 8;
    resource.maxSamples = 4;
    resource.limits.nonInductiveForLoops = true;
    resource.limits.whileLoops = true;
    resource.limits.doWhileLoops = true;
    resource.limits.generalUniformIndexing = true;
    resource.limits.generalAttributeMatrixVectorIndexing = 1;
    resource.limits.generalVaryingIndexing = true;
    resource.limits.generalSamplerIndexing = true;
    resource.limits.generalVariableIndexing = true;
    resource.limits.generalConstantMatrixVectorIndexing = true;
    return resource;
}

auto getEShLanguage(const std::string& fileExtension) -> EShLanguage {
    if (fileExtension == ".vert") {
        return EShLangVertex;
    } else if (fileExtension == ".tesc") {
        return EShLangTessControl;
    } else if (fileExtension == ".tese") {
        return EShLangTessEvaluation;
    } else if (fileExtension == ".geom") {
        return EShLangGeometry;
    } else if (fileExtension == ".frag") {
        return EShLangFragment;
    } else if (fileExtension == ".comp") {
        return EShLangCompute;
    } else {
        throw std::runtime_error("Unknown shader type: " + fileExtension);
    }
}

auto compileGLSLtoSPIRV(const std::string& sourceCode, EShLanguage shaderType)
    -> std::vector<uint32_t> {
    auto DefaultTBuiltInResource = getDefaultTBuiltInResource();
    const char* shaderStrings[1];
    shaderStrings[0] = sourceCode.c_str();

    glslang::TShader shader(shaderType);
    shader.setStrings(shaderStrings, 1);

    if (!shader.parse(&DefaultTBuiltInResource, 100, false, EShMsgDefault)) {
        throw std::runtime_error("GLSL Parsing Failed: " + std::string(shader.getInfoLog()));
    }

    glslang::TProgram program;
    program.addShader(&shader);

    if (!program.link(EShMsgDefault)) {
        throw std::runtime_error("GLSL Linking Failed: " + std::string(program.getInfoLog()));
    }

    std::vector<uint32_t> spirv;
    glslang::GlslangToSpv(*program.getIntermediate(shaderType), spirv);

    return spirv;
}
}  // namespace

namespace shader::compile {

void printShaderAttributes(std::span<const uint32_t> spirv) {
    spirv_cross::CompilerGLSL compiler(spirv.data(), spirv.size());
    auto resources = compiler.get_shader_resources();
    for (const auto& input : resources.stage_inputs) {
        uint32_t location = compiler.get_decoration(input.id, spv::DecorationLocation);
        spirv_cross::SPIRType type = compiler.get_type(input.type_id);

        std::string typeName;
        switch (type.basetype) {
            case spirv_cross::SPIRType::Float:
                if (type.vecsize == 1)
                    typeName = "float";
                else if (type.vecsize == 2)
                    typeName = "vec2";
                else if (type.vecsize == 3)
                    typeName = "vec3";
                else if (type.vecsize == 4)
                    typeName = "vec4";
                break;
            case spirv_cross::SPIRType::Int:
                if (type.vecsize == 1)
                    typeName = "int";
                else if (type.vecsize == 2)
                    typeName = "ivec2";
                else if (type.vecsize == 3)
                    typeName = "ivec3";
                else if (type.vecsize == 4)
                    typeName = "ivec4";
                break;
            // Add more cases as needed
            default:
                typeName = "unknown";
                break;
        }

        spdlog::debug("layout(location = {} in {} : {}", location, typeName, input.name);
    }

    // 打印绑定信息
    for (const auto& uniform_buffer : resources.uniform_buffers) {
        uint32_t binding = compiler.get_decoration(uniform_buffer.id, spv::DecorationBinding);
        uint32_t set = compiler.get_decoration(uniform_buffer.id, spv::DecorationDescriptorSet);
        spdlog::debug("layout(set = {} binding = {}) uniform_buffer.name {}", set, binding,
                      uniform_buffer.name);
    }

    for (const auto& sampled_image : resources.sampled_images) {
        uint32_t binding = compiler.get_decoration(sampled_image.id, spv::DecorationBinding);
        uint32_t set = compiler.get_decoration(sampled_image.id, spv::DecorationDescriptorSet);
        spdlog::debug("layout(set = {}, binding = {}) uniform sampler2D {}", set, binding,
                      sampled_image.name);
    }
}

auto getShaderInfo(std::span<const uint32_t> spirv) -> Info {
    Info info;

    spirv_cross::CompilerGLSL compiler(spirv.data(), spirv.size());
    auto resources = compiler.get_shader_resources();
    for (const auto& input : resources.stage_inputs) {
        uint32_t location = compiler.get_decoration(input.id, spv::DecorationLocation);
        spirv_cross::SPIRType type = compiler.get_type(input.type_id);

        std::string typeName;
        switch (type.basetype) {
            case spirv_cross::SPIRType::Float:
                if (type.vecsize == 1)
                    typeName = "float";
                else if (type.vecsize == 2)
                    typeName = "vec2";
                else if (type.vecsize == 3)
                    typeName = "vec3";
                else if (type.vecsize == 4)
                    typeName = "vec4";
                break;
            case spirv_cross::SPIRType::Int:
                if (type.vecsize == 1)
                    typeName = "int";
                else if (type.vecsize == 2)
                    typeName = "ivec2";
                else if (type.vecsize == 3)
                    typeName = "ivec3";
                else if (type.vecsize == 4)
                    typeName = "ivec4";
                break;
            // Add more cases as needed
            default:
                typeName = "unknown";
                break;
        }

        spdlog::debug("layout(location = {} in {} : {}", location, typeName, input.name);
    }

    // 打印绑定信息
    for (uint32_t index = 0; const auto& uniform_buffer : resources.uniform_buffers) {
        uint32_t binding = compiler.get_decoration(uniform_buffer.id, spv::DecorationBinding);
        uint32_t set = compiler.get_decoration(uniform_buffer.id, spv::DecorationDescriptorSet);

        info.constant_buffer_descriptors.push_back({index++, 1});
        spdlog::debug("layout(set = {} binding = {}) uniform_buffer.name {}", set, binding,
                      uniform_buffer.name);
    }

    for (const auto& sampled_image : resources.sampled_images) {
        uint32_t binding = compiler.get_decoration(sampled_image.id, spv::DecorationBinding);
        uint32_t set = compiler.get_decoration(sampled_image.id, spv::DecorationDescriptorSet);
        spdlog::debug("layout(set = {}, binding = {}) uniform sampler2D {}", set, binding,
                      sampled_image.name);
        TextureDescriptor td;
        td.count = 1;
        info.texture_descriptors.push_back(td);
    }
    return info;
}

void ShaderCompile::compile(const std::string_view& shader_path, std::string_view out_path) {
    auto shader_paths = list_files(shader_path);
    for (auto& path : shader_paths) {
        auto shader = read_shader(path);
        auto language = getEShLanguage(path.extension().string());
        auto spirv = compileGLSLtoSPIRV(shader, language);
        SPDLOG_INFO("Compiling: {}", path.string());
        auto out_file = std::string(out_path.data()) + path.filename().generic_string() + ".spv";
        std::ofstream out(out_file, std::ios::binary);
        out.write(reinterpret_cast<const char*>(spirv.data()), spirv.size() * sizeof(uint32_t));
    }
}
ShaderCompile::ShaderCompile() { glslang::InitializeProcess(); }
ShaderCompile::~ShaderCompile() { glslang::FinalizeProcess(); }

}  // namespace shader::compile
