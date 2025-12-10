#include "shader_compile.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>
#include <spirv_cross/spirv_cross.hpp>
#include <spirv_cross/spirv_glsl.hpp>

#include "absl/strings/match.h"

#include <spdlog/spdlog.h>
namespace {
namespace fs = std::filesystem;

auto read_shader(const fs::path& path) -> std::string {
    std::ifstream file(path, std::ios::ate | std::ios::binary);  // NOLINT
    auto file_size = file.tellg();
    std::string buffer(file_size, ' ');
    file.seekg(0);
    file.read(buffer.data(), file_size);
    return buffer;
}
auto getDefaultTBuiltInResource() -> TBuiltInResource* {
    static TBuiltInResource resource = {};
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
    return &resource;
}

auto getEShLanguage(const std::string& fileExtension) -> EShLanguage {
    if (fileExtension == ".vert") {
        return EShLangVertex;
    }
    if (fileExtension == ".tesc") {
        return EShLangTessControl;
    }
    if (fileExtension == ".tese") {
        return EShLangTessEvaluation;
    }
    if (fileExtension == ".geom") {
        return EShLangGeometry;
    }
    if (fileExtension == ".frag") {
        return EShLangFragment;
    }
    if (fileExtension == ".comp") {
        return EShLangCompute;
    }
    throw std::runtime_error("Unknown shader type: " + fileExtension);
}

auto compileGLSLtoSPIRV(const std::string& sourceCode, EShLanguage shaderType)
    -> std::vector<uint32_t> {
    glslang::TShader shader(shaderType);

    const char* shaderStrings[] = {sourceCode.c_str()};  // NOLINT
    shader.setStrings(shaderStrings, 1);                 // NOLINT

    // 2. 设置语言标准（GLSL 450）
    shader.setEnvInput(glslang::EShSourceGlsl, shaderType, glslang::EShClientVulkan, 100);
    shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_0);
    shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_0);

    // 4. 编译
    auto messages = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);
    bool success = shader.parse(getDefaultTBuiltInResource(), 100, false, messages);

    if (!success) {
        SPDLOG_ERROR("build shader{}", shader.getInfoLog());
        return {};
    }
    // 5. 创建程序对象并链接（对于单个着色器，只需添加）
    glslang::TProgram program;
    program.addShader(&shader);

    // 6. 输出 SPIR-V
    spv::SpvBuildLogger logger;
    glslang::SpvOptions spvOptions;
    spvOptions.optimizeSize = true;
    spvOptions.generateDebugInfo = false;

    std::vector<uint32_t> spirv;
    glslang::GlslangToSpv(*program.getIntermediate(shaderType), spirv, &logger, &spvOptions);
    SPDLOG_INFO("build shader:\n {}", logger.getAllMessages());
    return spirv;
}

auto GetTextureType(const spirv_cross::SPIRType& type) -> shader::TextureType {
    using Dim = spv::Dim;

    bool is_array = type.image.arrayed != 0;
    Dim dim = type.image.dim;

    switch (dim) {
        case Dim::Dim1D:
            return is_array ? shader::TextureType::ColorArray1D : shader::TextureType::Color1D;

        case Dim::Dim2D:
            return is_array ? shader::TextureType::ColorArray2D : shader::TextureType::Color2D;

        case Dim::Dim3D:
            // 3D 纹理不能是数组（GL/Vulkan 中不允许）
            return shader::TextureType::Color3D;

        case Dim::DimCube:
            return is_array ? shader::TextureType::ColorArrayCube : shader::TextureType::ColorCube;

        case Dim::DimBuffer:
            return shader::TextureType::Buffer;  // ← Uniform Texel Buffer

        default:
            return shader::TextureType::Color2D;  // fallback
    }
}

auto IsRectTexture(const spirv_cross::Resource& resource) -> bool {
    const std::string& name = resource.name;
    // 常见命名：_rect, Rect, sampler2DRect, 甚至包含 "rect" 的名字
    std::string lower_name = name;
    // msvc c4242 error fix
    std::ranges::transform(lower_name, lower_name.begin(),
                           [](auto c) { return static_cast<char>(::tolower(c)); });

    return absl::StrContains(lower_name, "rect");
}

auto GetTextureBufferPixelFormat(const spirv_cross::SPIRType& sampled_type)
    -> shader::TexturePixelFormat {
    using BaseType = spirv_cross::SPIRType::BaseType;

    switch (sampled_type.basetype) {
        case BaseType::Float:
            switch (sampled_type.vecsize) {
                case 1:
                    return shader::TexturePixelFormat::R32_FLOAT;
                case 2:
                    return shader::TexturePixelFormat::R32G32_FLOAT;
                case 3:
                    return shader::TexturePixelFormat::R32G32B32_FLOAT;
                case 4:
                    return shader::TexturePixelFormat::R32G32B32A32_FLOAT;
            }
            break;

        case BaseType::Int:
            switch (sampled_type.vecsize) {
                case 1:
                    return shader::TexturePixelFormat::R32_SINT;
                case 2:
                    return shader::TexturePixelFormat::R32G32_SINT;
                case 3:
                    return shader::TexturePixelFormat::R32G32B32_SINT;
                case 4:
                    return shader::TexturePixelFormat::R32G32B32A32_SINT;
            }
            break;

        case BaseType::UInt:
            switch (sampled_type.vecsize) {
                case 1:
                    return shader::TexturePixelFormat::R32_UINT;
                case 2:
                    return shader::TexturePixelFormat::R32G32_UINT;
                case 3:
                    return shader::TexturePixelFormat::R32G32B32_UINT;
                case 4:
                    return shader::TexturePixelFormat::R32G32B32A32_UINT;
            }
            break;

        default:
            // fallback
            return shader::TexturePixelFormat::R32G32B32A32_FLOAT;
    }

    // fallback
    return shader::TexturePixelFormat::R32G32B32A32_FLOAT;
}

auto GetImageTextureType(const spirv_cross::SPIRType& type) -> shader::TextureType {
    bool is_array = type.image.arrayed;
    bool is_cube = (type.image.dim == spv::Dim::DimCube);

    switch (type.image.dim) {
        case spv::Dim::Dim1D:
            return is_array ? shader::TextureType::ColorArray1D : shader::TextureType::Color1D;
        case spv::Dim::Dim2D:
            if (is_cube) {
                return is_array ? shader::TextureType::ColorArrayCube
                                : shader::TextureType::ColorCube;
            }
            return is_array ? shader::TextureType::ColorArray2D : shader::TextureType::Color2D;
        case spv::Dim::Dim3D:
            return shader::TextureType::Color3D;
        case spv::Dim::DimBuffer:
            return shader::TextureType::Buffer;  // 不应出现在 storage_image
        default:
            return shader::TextureType::Color2D;  // fallback
    }
}

auto GetImageFormat(const spirv_cross::Compiler& compiler, const spirv_cross::SPIRType& type)
    -> shader::ImageFormat {
    const auto& sampled_type = compiler.get_type(type.image.type);

// 使用兼容宏（如之前定义）
#ifdef SPIRV_CROSS_BASETYPE_SINT
    auto SInt = SPIRV_CROSS_BASETYPE_SINT;
#else
    auto SInt = spirv_cross::SPIRType::BaseType::Int;
#endif

    auto UInt = spirv_cross::SPIRType::BaseType::UInt;
    auto Float = spirv_cross::SPIRType::BaseType::Float;

    if (sampled_type.basetype == Float) {
        switch (sampled_type.vecsize) {
            case 4:
                return shader::ImageFormat::R32G32B32A32_FLOAT;
            default:
                return shader::ImageFormat::Typeless;
        }
    } else if (sampled_type.basetype == SInt) {
        switch (sampled_type.vecsize) {
            case 1:
                return shader::ImageFormat::R32_SINT;
            case 2:
                return shader::ImageFormat::R32G32_SINT;
            case 4:
                return shader::ImageFormat::R32G32B32A32_SINT;
            default:
                return shader::ImageFormat::Typeless;
        }
    } else if (sampled_type.basetype == UInt) {
        switch (sampled_type.vecsize) {
            case 1:
                return shader::ImageFormat::R32_UINT;
            case 2:
                return shader::ImageFormat::R16_UINT;
            case 4:
                return shader::ImageFormat::R32G32B32A32_UINT;
            default:
                return shader::ImageFormat::Typeless;
        }
    }

    return shader::ImageFormat::Typeless;
}

auto GetImageBufferFormat(const spirv_cross::SPIRType& sampled_type) -> shader::ImageFormat {
    using BaseType = spirv_cross::SPIRType::BaseType;

// 兼容 SInt / Int
#if defined(SPIRV_CROSS_VERSION_MAJOR) && \
    (SPIRV_CROSS_VERSION_MAJOR > 1 ||     \
     (SPIRV_CROSS_VERSION_MAJOR == 1 && SPIRV_CROSS_VERSION_MINOR >= 3))
    const auto SInt = BaseType::SInt;
#else
    const auto SInt = BaseType::Int;
#endif

    const auto UInt = BaseType::UInt;
    const auto Float = BaseType::Float;

    if (sampled_type.basetype == SInt) {
        switch (sampled_type.vecsize) {
            case 1:
                return shader::ImageFormat::R32_SINT;
            case 2:
                return shader::ImageFormat::R32G32_SINT;
            case 4:
                return shader::ImageFormat::R32G32B32A32_SINT;
            default:
                break;
        }
    } else if (sampled_type.basetype == UInt) {
        switch (sampled_type.vecsize) {
            case 1:
                return shader::ImageFormat::R32_UINT;
            case 2:
                return shader::ImageFormat::R32G32_UINT;
            case 4:
                return shader::ImageFormat::R32G32B32A32_UINT;
            default:
                break;
        }
    } else if (sampled_type.basetype == Float) {
        switch (sampled_type.vecsize) {
            case 1:
                return shader::ImageFormat::R32_FLOAT;
            case 2:
                return shader::ImageFormat::R32G32_FLOAT;
            case 4:
                return shader::ImageFormat::R32G32B32A32_FLOAT;
            default:
                break;
        }
    }

    return shader::ImageFormat::Typeless;  // fallback
}

}  // namespace

namespace shader::compile {

auto getShaderInfo(std::span<const uint32_t> spirv) -> Info {
    Info info;
    const spirv_cross::Compiler compiler(spirv.data(), spirv.size());
    auto resources = compiler.get_shader_resources();
    std::unordered_map<std::uint32_t, std::uint32_t> bindingStrideMap;

    // uniform_buffer_descriptors
    for (const auto& resource : resources.uniform_buffers) {
        uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
        uint32_t set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
        uint32_t count = 1;
        // 获取该变量的类型
        const spirv_cross::SPIRType& type = compiler.get_type(resource.type_id);

        if (type.array.size() > 0) {
            // 如果是数组，则取第一个维度的大小
            count = static_cast<uint32_t>(compiler.get_constant(type.array[0]).scalar());
        }

        info.uniform_buffer_descriptors.push_back({.binding = binding, .count = count, .set = set});
    }

    // storage_buffers_descriptors
    for (const auto& resource : resources.storage_buffers) {
        uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
        uint32_t set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
        uint32_t count = 1;
        // 获取该变量的类型
        const spirv_cross::SPIRType& type = compiler.get_type(resource.type_id);

        if (type.array.size() > 0) {
            // 如果是数组，则取第一个维度的大小
            count = static_cast<uint32_t>(compiler.get_constant(type.array[0]).scalar());
        }
        info.storage_buffers_descriptors.push_back(
            {.binding = binding, .count = count, .set = set, .is_written = false});
    }
    // eCombinedImageSampler
    for (const auto& resource : resources.sampled_images) {
        uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
        uint32_t set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
        uint32_t count = 1;
        // 获取该变量的类型
        const spirv_cross::SPIRType& type = compiler.get_type(resource.type_id);
        if (type.array.size() > 0) {
            // 如果是数组，则取第一个维度的大小
            count = static_cast<uint32_t>(compiler.get_constant(type.array[0]).scalar());
        }
        TextureDescriptor td{};
        td.count = count;
        td.set = set;
        td.binding = binding;
        td.is_multisample = (type.image.ms != 0);
        td.is_depth = (type.image.depth != 0);
        if (IsRectTexture(resource)) {
            td.type = TextureType::Color2DRect;
        } else {
            td.type = GetTextureType(type);
        }
        info.texture_descriptors.push_back(td);
    }

    // eUniformTexelBuffer
    for (const auto& resource : resources.separate_samplers) {
        const auto& type = compiler.get_type(resource.type_id);

        uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
        uint32_t set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
        uint32_t count = 1;
        if (!type.array.empty()) {
            count = static_cast<uint32_t>(compiler.get_constant(type.array[0]).scalar());
        }

        // 获取样本类型
        const auto& sampled_type = compiler.get_type(type.image.type);

        shader::TexturePixelFormat format = GetTextureBufferPixelFormat(sampled_type);

        shader::TextureBufferDescriptor desc{
            .binding = binding, .count = count, .set = set, .format = format};

        // 添加到你的描述符列表
        info.texture_buffer_descriptors.push_back(desc);
    }

    // eStorageImage
    for (const auto& resource : compiler.get_shader_resources().storage_images) {
        const auto& type = compiler.get_type(resource.type_id);

        uint32_t set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);

        uint32_t binding = compiler.get_decoration(resource.id, spv::DecorationBinding);

        // ✅ 数组大小
        uint32_t count = 1;
        if (!type.array.empty()) {
            count = static_cast<uint32_t>(compiler.get_constant(type.array[0]).scalar());
        }

        // ✅ 维度和类型
        TextureType tex_type = GetImageTextureType(type);
        if (tex_type == TextureType::Buffer) {
            continue;  // 不应是 buffer
        }

        // ✅ 格式映射
        ImageFormat format = GetImageFormat(compiler, type);

        // 注意：storage image 默认可读可写，但可通过访问模式判断
        bool is_written = true;  // storage image 默认可写
        bool is_read = true;     // 默认可读

        // 可选：通过访问模式更精确判断（SPIRV-Cross 支持有限）
        // 这里我们假设所有 storage image 都是读写

        info.image_descriptors.push_back({
            .type = tex_type,
            .format = format,
            .is_written = is_written,
            .is_read = is_read,
            .binding = binding,
            .count = count,
            .set = set,
        });
    }

    // 获取所有 storage images（包括 image2D, imageBuffer 等）

    // eStorageTexelBuffer
    for (const auto& resource : compiler.get_shader_resources().storage_images) {
        const auto& type = compiler.get_type(resource.type_id);

        // ✅ 只处理 Buffer 类型：即 GLSL 的 imageBuffer
        if (type.image.dim != spv::Dim::DimBuffer) {
            continue;
        }

        // ✅ 获取 set 和 binding
        uint32_t set = 0;
        uint32_t binding = 0;

        try {
            set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
            binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
        } catch (...) {
            continue;  // 装饰缺失，跳过
        }

        // ✅ 数组大小（如 imageBuffer arr[4]）
        uint32_t count = 1;
        if (!type.array.empty()) {
            // 数组大小是常量表达式，从常量中获取
            const auto& array_size_const = compiler.get_constant(type.array[0]);
            count = static_cast<uint32_t>(array_size_const.scalar());
        }

        // ✅ 获取样本数据类型（vec4<int>, float, etc.）
        const auto& sampled_type = compiler.get_type(type.image.type);

        // ✅ 推断 ImageFormat
        ImageFormat format = GetImageBufferFormat(sampled_type);
        if (format == ImageFormat::Typeless) {
            // 可选：日志警告
            // printf("Warning: Unsupported imageBuffer format for binding %u\n", binding);
            continue;
        }

        // ✅ 是否可写/可读
        // storage image 默认可读可写
        bool is_written = true;
        bool is_read = true;

        // 可选：通过访问模式进一步判断（SPIRV-Cross 支持有限）
        // 这里假设所有 storage image 都是读写

        info.image_buffer_descriptors.push_back({
            .format = format,
            .is_written = is_written,
            .is_read = is_read,
            .binding = binding,
            .count = count,
            .set = set,
        });
    }

    for (const auto& resource : compiler.get_shader_resources().push_constant_buffers) {
        const auto& type = compiler.get_type(resource.type_id);
        auto size = static_cast<uint32_t>(compiler.get_declared_struct_size(type));
        info.push_constants.size = size;
    }

    return info;
}

// NOLINTNEXTLINE
auto ShaderCompile::compile(const std::string_view& shader_path) -> std::vector<uint32_t> {
    fs::path path{shader_path};
    auto shader = read_shader(path);
    auto language = getEShLanguage(path.extension().string());
    return compileGLSLtoSPIRV(shader, language);
}
ShaderCompile::ShaderCompile() {
    if (!glslang::InitializeProcess()) {
        throw std::runtime_error("Failed to initialize glslang");
    }
}
ShaderCompile::~ShaderCompile() { glslang::FinalizeProcess(); }

}  // namespace shader::compile
