#include "device.hpp"
#include <spdlog/spdlog.h>
#include <common/settings.hpp>
#include <unordered_set>
#include "common/literals.hpp"
namespace render::vulkan {
namespace {
template <typename T>
void SetNext(void**& next, T& data) {
    *next = &data;
    next = &data.pNext;
}
}  // namespace
Device::Device(vk::Instance instance, vk::PhysicalDevice physical, vk::SurfaceKHR surface,
               bool enable_validation)
    : instance_(instance), physical_(physical) {
    format_properties_ = utils::GetFormatProperties(physical_);
    const bool is_suitable = getSuitability(surface != nullptr);
    const vk::DriverId driver_id = static_cast<vk::DriverId>(properties_.driver_.driverID);
    const auto device_id = properties_.properties_.deviceID;
    const bool is_radv = driver_id == ::vk::DriverId::eMesaRadv;
    const bool is_amd_driver =
        driver_id == ::vk::DriverId::eAmdProprietary || driver_id == ::vk::DriverId::eAmdOpenSource;
    const bool is_amd = is_amd_driver || is_radv;
    const bool is_intel_windows = driver_id == ::vk::DriverId::eIntelProprietaryWindows;
    const bool is_intel_anv = driver_id == ::vk::DriverId::eIntelOpenSourceMESA;
    const bool is_nvidia = driver_id == ::vk::DriverId::eNvidiaProprietary;
    const bool is_mvk = driver_id == ::vk::DriverId::eMoltenvk;
    const bool is_qualcomm = driver_id == ::vk::DriverId::eQualcommProprietary;
    const bool is_turnip = driver_id == ::vk::DriverId::eMesaTurnip;
    const bool is_s8gen2 = device_id == 0x43050a01;
    const bool is_arm = driver_id == ::vk::DriverId::eArmProprietary;
    if ((is_mvk || is_qualcomm || is_turnip || is_arm) && !is_suitable) {
        SPDLOG_WARN("Unsuitable driver, continuing anyway");
    } else if (!is_suitable) {
        throw std::runtime_error("Unsuitable driver");
    }
    if (is_nvidia) {
        nvidia_arch = utils::getNvidiaArchitecture(physical, supported_extensions_);
    }
    setupFamilies(surface);
    const auto queue_cis = getDeviceQueueCreateInfos();
    // GetSuitability has already configured the linked list of features for us.
    // Reuse it here.
    const void* first_next = &features2_;
    misc_features_.is_blit_depth24_stencil8_supported =
        testDepthStencilBlits(vk::Format::eD24UnormS8Uint);
    misc_features_.is_blit_depth32_stencil8_supported =
        testDepthStencilBlits(vk::Format::eD32SfloatS8Uint);
    vk::PhysicalDeviceProperties properties = properties_.properties_;
    misc_features_.is_optimal_astc_supported = computeIsOptimalAstcSupported();
    misc_features_.is_warp_potentially_bigger =
        !extensions_.subgroup_size_control ||
        properties_.subgroup_size_control_.maxSubgroupSize > GUEST_WARP_SIZE;

    misc_features_.is_integrated =
        properties.deviceType ==
        vk::PhysicalDeviceType::eIntegratedGpu;  // VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;
    misc_features_.is_virtual =
        properties.deviceType ==
        vk::PhysicalDeviceType::eVirtualGpu;  // VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU;
    misc_features_.is_non_gpu =
        properties.deviceType == vk::PhysicalDeviceType::eOther ||
        properties.deviceType == vk::PhysicalDeviceType::eCpu;  // VK_PHYSICAL_DEVICE_TYPE_CPU;

    misc_features_.supports_d24_depth = isFormatSupported(
        vk::Format::eD24UnormS8Uint,  // VK_FORMAT_D24_UNORM_S8_UINT,
        vk::FormatFeatureFlagBits::eDepthStencilAttachment,
        utils::FormatType::Optimal);  // VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT

    misc_features_.supports_conditional_barriers = !(is_intel_anv || is_intel_windows);
    collectPhysicalMemoryInfo();
    collectToolingInfo();
    if (is_qualcomm || is_turnip) {
        SPDLOG_WARN("Qualcomm and Turnip drivers have broken VK_EXT_custom_border_color");
        removeExtensionFeature(extensions_.custom_border_color, features_.custom_border_color,
                               VK_EXT_CUSTOM_BORDER_COLOR_EXTENSION_NAME);
    }
    if (is_qualcomm) {
        misc_features_.must_emulate_scaled_formats = true;

        SPDLOG_WARN("Qualcomm drivers have broken VK_EXT_extended_dynamic_state");
        removeExtensionFeature(extensions_.extended_dynamic_state, features_.extended_dynamic_state,
                               VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME);

        SPDLOG_WARN("Qualcomm drivers have a slow VK_KHR_push_descriptor implementation");
        removeExtension(extensions_.push_descriptor, VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME);
    }
    if (is_arm) {
        misc_features_.must_emulate_scaled_formats = true;

        SPDLOG_WARN("ARM drivers have broken VK_EXT_extended_dynamic_state");
        removeExtensionFeature(extensions_.extended_dynamic_state, features_.extended_dynamic_state,
                               VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME);
    }

    if (is_nvidia) {
        const uint32_t nv_major_version = (properties_.properties_.driverVersion >> 22) & 0x3ff;
        const auto arch = nvidia_arch;
        if (arch >= utils::NvidiaArchitecture::Arch_AmpereOrNewer) {
            SPDLOG_WARN("Ampere and newer have broken float16 math");
            features_.shader_float16_int8.shaderFloat16 = false;
        } else if (arch <= utils::NvidiaArchitecture::Arch_Volta) {
            if (nv_major_version < 527) {
                SPDLOG_WARN("{} Volta and older have broken VK_KHR_push_descriptor",
                            "Render_Vulkan");
                removeExtension(extensions_.push_descriptor, VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME);
            }
        }
        if (nv_major_version >= 510) {
            SPDLOG_WARN("{} NVIDIA Drivers >= 510 do not support MSAA image blits",
                        "Render_Vulkan");
            misc_features_.cant_blit_msaa = true;
        }
    }

    if (extensions_.extended_dynamic_state && is_radv) {
        // Mask driver version variant
        const uint32_t version = (properties_.properties_.driverVersion << 3) >> 3;
        if (version < VK_MAKE_API_VERSION(0, 21, 2, 0)) {
            SPDLOG_WARN(
                "Render_Vulkan: RADV versions older than 21.2 have broken "
                "VK_EXT_extended_dynamic_state");
            removeExtensionFeature(extensions_.extended_dynamic_state,
                                   features_.extended_dynamic_state,
                                   VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME);
        }
    }

    if (extensions_.extended_dynamic_state2 && is_radv) {
        const uint32_t version = (properties_.properties_.driverVersion << 3) >> 3;
        if (version < VK_MAKE_API_VERSION(0, 22, 3, 1)) {
            SPDLOG_WARN(
                "RADV versions older than 22.3.1 have broken VK_EXT_extended_dynamic_state2");
            removeExtensionFeature(extensions_.extended_dynamic_state2,
                                   features_.extended_dynamic_state2,
                                   VK_EXT_EXTENDED_DYNAMIC_STATE_2_EXTENSION_NAME);
        }
    }

    if (extensions_.extended_dynamic_state2 && is_qualcomm) {
        const uint32_t version = (properties_.properties_.driverVersion << 3) >> 3;
        if (version >= VK_MAKE_API_VERSION(0, 0, 676, 0) &&
            version < VK_MAKE_API_VERSION(0, 0, 680, 0)) {
            // Qualcomm Adreno 7xx drivers do not properly support extended_dynamic_state2.
            SPDLOG_WARN("Qualcomm Adreno 7xx drivers have broken VK_EXT_extended_dynamic_state2");
            removeExtensionFeature(extensions_.extended_dynamic_state2,
                                   features_.extended_dynamic_state2,
                                   VK_EXT_EXTENDED_DYNAMIC_STATE_2_EXTENSION_NAME);
        }
    }
    // Mesa RadV drivers still have broken extendedDynamicState3ColorBlendEquation support.
    if (extensions_.extended_dynamic_state3 && is_radv) {
        SPDLOG_WARN("RADV has broken extendedDynamicState3ColorBlendEquation");
        features_.extended_dynamic_state3.extendedDynamicState3ColorBlendEnable = false;
        features_.extended_dynamic_state3.extendedDynamicState3ColorBlendEquation = false;
        misc_features_.dynamic_state3_blending = false;

        const uint32_t version = (properties_.properties_.driverVersion << 3) >> 3;
        if (version < VK_MAKE_API_VERSION(0, 23, 1, 0)) {
            SPDLOG_WARN("RADV versions older than 23.1.0 have broken depth clamp dynamic state");
            features_.extended_dynamic_state3.extendedDynamicState3DepthClampEnable = false;
            misc_features_.dynamic_state3_enables = false;
        }
    }

    // AMD still has broken extendedDynamicState3ColorBlendEquation on RDNA3.
    // TODO: distinguis RDNA3 from other uArchs.
    if (extensions_.extended_dynamic_state3 && is_amd_driver) {
        SPDLOG_WARN("AMD drivers have broken extendedDynamicState3ColorBlendEquation");
        features_.extended_dynamic_state3.extendedDynamicState3ColorBlendEnable = false;
        features_.extended_dynamic_state3.extendedDynamicState3ColorBlendEquation = false;
        misc_features_.dynamic_state3_blending = false;
    }

    if (extensions_.vertex_input_dynamic_state && is_radv) {
        // TODO(ameerj): Blacklist only offending driver versions
        // TODO(ameerj): Confirm if RDNA1 is affected
        const bool is_rdna2 =
            supported_extensions_.contains(VK_KHR_FRAGMENT_SHADING_RATE_EXTENSION_NAME);
        if (is_rdna2) {
            SPDLOG_WARN("RADV has broken VK_EXT_vertex_input_dynamic_state on RDNA2 hardware");
            removeExtensionFeature(extensions_.vertex_input_dynamic_state,
                                   features_.vertex_input_dynamic_state,
                                   VK_EXT_VERTEX_INPUT_DYNAMIC_STATE_EXTENSION_NAME);
        }
    }

    if (extensions_.vertex_input_dynamic_state && is_qualcomm) {
        // Qualcomm drivers do not properly support vertex_input_dynamic_state.
        SPDLOG_WARN("Qualcomm drivers have broken VK_EXT_vertex_input_dynamic_state");
        removeExtensionFeature(extensions_.vertex_input_dynamic_state,
                               features_.vertex_input_dynamic_state,
                               VK_EXT_VERTEX_INPUT_DYNAMIC_STATE_EXTENSION_NAME);
    }
    sets_per_pool_ = 64;

    if (is_amd_driver) {
        // AMD drivers need a higher amount of Sets per Pool in certain circumstances like in XC2.
        sets_per_pool_ = 96;
        // Disable VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT on AMD GCN4 and lower as it is broken.
        if (!features_.shader_float16_int8.shaderFloat16) {
            SPDLOG_WARN("AMD GCN4 and earlier have broken VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT");
            misc_features_.has_broken_cube_compatibility = true;
        }
    }

    if (is_qualcomm) {
        const uint32_t version = (properties_.properties_.driverVersion << 3) >> 3;
        if (version < VK_MAKE_API_VERSION(0, 255, 615, 512)) {
            misc_features_.has_broken_parallel_compiling = true;
        }
    }
    if (extensions_.sampler_filter_minmax && is_amd) {
        // Disable ext_sampler_filter_minmax on AMD GCN4 and lower as it is broken.
        if (!features_.shader_float16_int8.shaderFloat16) {
            SPDLOG_WARN("AMD GCN4 and earlier have broken VK_EXT_sampler_filter_minmax");
            removeExtension(extensions_.sampler_filter_minmax,
                            VK_EXT_SAMPLER_FILTER_MINMAX_EXTENSION_NAME);
        }
    }

    if (extensions_.vertex_input_dynamic_state && is_intel_windows) {
        const uint32_t version = (properties_.properties_.driverVersion << 3) >> 3;
        if (version < VK_MAKE_API_VERSION(27, 20, 100, 0)) {
            SPDLOG_WARN("Intel has broken VK_EXT_vertex_input_dynamic_state");
            removeExtensionFeature(extensions_.vertex_input_dynamic_state,
                                   features_.vertex_input_dynamic_state,
                                   VK_EXT_VERTEX_INPUT_DYNAMIC_STATE_EXTENSION_NAME);
        }
    }

    if (features_.shader_float16_int8.shaderFloat16 && is_intel_windows) {
        // Intel's compiler crashes when using fp16 on Astral Chain, disable it for the time being.
        SPDLOG_WARN("Intel has broken float16 math");
        features_.shader_float16_int8.shaderFloat16 = false;
    }
    if (is_intel_windows) {
        SPDLOG_WARN("Intel proprietary drivers do not support MSAA image blits");
        misc_features_.cant_blit_msaa = true;
    }

    auto vk_setting = common::settings::get<settings::RenderVulkan>();
    misc_features_.has_broken_compute =
        utils::checkBrokenCompute(static_cast<vk::DriverId>(properties_.driver_.driverID),
                                  properties_.properties_.driverVersion) &&
        !vk_setting.enable_compute_pipelines;
    if (is_intel_anv || (is_qualcomm && !is_s8gen2)) {
        SPDLOG_WARN("Driver does not support native BGR format");
        misc_features_.must_emulate_bgr565 = true;
    }

    if (extensions_.push_descriptor && is_intel_anv) {
        const uint32_t version = (properties_.properties_.driverVersion << 3) >> 3;
        if (version >= VK_MAKE_API_VERSION(0, 22, 3, 0) &&
            version < VK_MAKE_API_VERSION(0, 23, 2, 0)) {
            // Disable VK_KHR_push_descriptor due to
            // mesa/mesa/-/commit/ff91c5ca42bc80aa411cb3fd8f550aa6fdd16bdc
            SPDLOG_WARN("ANV drivers 22.3.0 to 23.1.0 have broken VK_KHR_push_descriptor");
            removeExtension(extensions_.push_descriptor, VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME);
        }
    } else if (extensions_.push_descriptor && is_nvidia) {
        const auto arch = nvidia_arch;
        if (arch <= utils::NvidiaArchitecture::Arch_Pascal) {
            SPDLOG_WARN("Pascal and older architectures have broken VK_KHR_push_descriptor");
            removeExtension(extensions_.push_descriptor, VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME);
        }
    }

    if (is_mvk) {
        SPDLOG_WARN("MVK driver breaks when using more than 16 vertex attributes/bindings");
        properties_.properties_.limits.maxVertexInputAttributes =
            std::min(properties_.properties_.limits.maxVertexInputAttributes, 16U);
        properties_.properties_.limits.maxVertexInputBindings =
            std::min(properties_.properties_.limits.maxVertexInputBindings, 16U);
    }

    if (is_turnip) {
        SPDLOG_WARN("Turnip requires higher-than-reported binding limits");
        properties_.properties_.limits.maxVertexInputBindings = 32;
    }

    if (!extensions_.extended_dynamic_state && extensions_.extended_dynamic_state2) {
        SPDLOG_WARN("Removing extendedDynamicState2 due to missing extendedDynamicState");
        removeExtensionFeature(extensions_.extended_dynamic_state2,
                               features_.extended_dynamic_state2,
                               VK_EXT_EXTENDED_DYNAMIC_STATE_2_EXTENSION_NAME);
    }
    if (!extensions_.extended_dynamic_state2 && extensions_.extended_dynamic_state3) {
        SPDLOG_WARN("Removing extendedDynamicState3 due to missing extendedDynamicState2");
        removeExtensionFeature(extensions_.extended_dynamic_state3,
                               features_.extended_dynamic_state3,
                               VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME);
        misc_features_.dynamic_state3_blending = false;
        misc_features_.dynamic_state3_enables = false;
    }

    vk::DeviceCreateInfo ci{};
    auto device_extends = utils::extensionListForVulkan(loaded_extensions_);
    ci.setQueueCreateInfos(queue_cis)
        .setPEnabledExtensionNames(device_extends)
        .setPNext(first_next);

    if (enable_validation) {
        const ::std::array<const char*, 1> validationLayers = {"VK_LAYER_KHRONOS_validation"};
        ci.setPEnabledLayerNames(validationLayers);
    }

    logical_ = physical_.createDevice(ci);

    graphics_queue_ = logical_.getQueue(graphics_family_, 0);
    present_queue_ = logical_.getQueue(present_family_, 0);
    compute_queue_ = logical_.getQueue(compute_family_, 0);

    VmaVulkanFunctions functions{};
    functions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
    functions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;
    const VmaAllocatorCreateInfo allocator_info = {
        .flags = VMA_ALLOCATOR_CREATE_EXTERNALLY_SYNCHRONIZED_BIT,
        .physicalDevice = physical,
        .device = logical_,
        .preferredLargeHeapBlockSize = 0,
        .pAllocationCallbacks = nullptr,
        .pDeviceMemoryCallbacks = nullptr,
        .pHeapSizeLimit = nullptr,
        .pVulkanFunctions = &functions,
        .instance = instance,
        .vulkanApiVersion = VK_API_VERSION_1_1,
        .pTypeExternalMemoryHandleTypes = nullptr,
    };

    vmaCreateAllocator(&allocator_info, &allocator_);
}
Device::~Device() { vmaDestroyAllocator(allocator_); }
auto Device::getSuitability(bool requires_swapchain) -> bool {
    // Assume we will be suitable
    bool suitable = true;

    properties_.properties_ = physical_.getProperties();

    // Set instance version.
    instance_version_ = properties_.properties_.apiVersion;
    // Minimum of API version 1.1 is required. (This is well-supported.)
    assert(instance_version_ >= VK_API_VERSION_1_1);
    // Get available extensions.
    auto extension_properties = physical_.enumerateDeviceExtensionProperties();

    // Get the set of supported extensions.
    supported_extensions_.clear();
    for (const vk::ExtensionProperties& property : extension_properties) {
        supported_extensions_.insert(property.extensionName);
    }
    // Generate list of extensions to load.
    loaded_extensions_.clear();

#define EXTENSION(prefix, macro_name, var_name)                                        \
    if (supported_extensions_.contains(VK_##prefix##_##macro_name##_EXTENSION_NAME)) { \
        loaded_extensions_.insert(VK_##prefix##_##macro_name##_EXTENSION_NAME);        \
        extensions_.var_name = true;                                                   \
    }
#define FEATURE_EXTENSION(prefix, struct_name, macro_name, var_name)                   \
    if (supported_extensions_.contains(VK_##prefix##_##macro_name##_EXTENSION_NAME)) { \
        loaded_extensions_.insert(VK_##prefix##_##macro_name##_EXTENSION_NAME);        \
        extensions_.var_name = true;                                                   \
    }

    if (instance_version_ < VK_API_VERSION_1_2) {
        FOR_EACH_VK_FEATURE_1_2(FEATURE_EXTENSION);
    }
    if (instance_version_ < VK_API_VERSION_1_3) {
        FOR_EACH_VK_FEATURE_1_3(FEATURE_EXTENSION);
    }

    FOR_EACH_VK_FEATURE_EXT(FEATURE_EXTENSION);
    FOR_EACH_VK_EXTENSION(EXTENSION);

#undef FEATURE_EXTENSION
#undef EXTENSION

    // Some extensions are mandatory. Check those.
#define CHECK_EXTENSION(extension_name)                                \
    if (!loaded_extensions_.contains(extension_name)) {                \
        SPDLOG_ERROR("Missing required extension {}", extension_name); \
        suitable = false;                                              \
    }

#define LOG_EXTENSION(extension_name)                                       \
    if (!loaded_extensions_.contains(extension_name)) {                     \
        SPDLOG_INFO("Device doesn't support extension {}", extension_name); \
    }

    FOR_EACH_VK_RECOMMENDED_EXTENSION(LOG_EXTENSION);
    FOR_EACH_VK_MANDATORY_EXTENSION(CHECK_EXTENSION);

    if (requires_swapchain) {
        CHECK_EXTENSION(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    }

#undef LOG_EXTENSION
#undef CHECK_EXTENSION
    // Generate the linked list of features to test.
    features2_.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    // Set next pointer.
    void** next = &features2_.pNext;

    // Test all features we know about. If the feature is not available in core at our
// current API version, and was not enabled by an extension, skip testing the feature.
// We set the structure sType explicitly here as it is zeroed by the constructor.
#define FEATURE(prefix, struct_name, macro_name, var_name)                                \
    features_.var_name.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_##macro_name##_FEATURES; \
    SetNext(next, features_.var_name);

#define EXT_FEATURE(prefix, struct_name, macro_name, var_name)                  \
    if (extensions_.var_name) {                                                 \
        features_.var_name.sType =                                              \
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_##macro_name##_FEATURES_##prefix; \
        SetNext(next, features_.var_name);                                      \
    }

    FOR_EACH_VK_FEATURE_1_1(FEATURE);
    FOR_EACH_VK_FEATURE_EXT(EXT_FEATURE);
    if (instance_version_ >= VK_API_VERSION_1_2) {
        FOR_EACH_VK_FEATURE_1_2(FEATURE);
    } else {
        FOR_EACH_VK_FEATURE_1_2(EXT_FEATURE);
    }
    if (instance_version_ >= VK_API_VERSION_1_3) {
        FOR_EACH_VK_FEATURE_1_3(FEATURE);
    } else {
        FOR_EACH_VK_FEATURE_1_3(EXT_FEATURE);
    }

#undef EXT_FEATURE
#undef FEATURE
    vkGetPhysicalDeviceFeatures2(physical_, &features2_);
    features_.features = features2_.features;

    // Some features are mandatory. Check those.
#define CHECK_FEATURE(feature, name)                        \
    if (!features_.feature.name) {                          \
        SPDLOG_ERROR("Missing required feature {}", #name); \
        suitable = false;                                   \
    }

#define LOG_FEATURE(feature, name)                               \
    if (!features_.feature.name) {                               \
        SPDLOG_INFO("Device doesn't support feature {}", #name); \
    }

    FOR_EACH_VK_RECOMMENDED_FEATURE(LOG_FEATURE);
    FOR_EACH_VK_MANDATORY_FEATURE(CHECK_FEATURE);

#undef LOG_FEATURE
#undef CHECK_FEATURE

    // Set next pointer.
    next = &properties2_.pNext;
    SetNext(next, properties_.driver_);
    SetNext(next, properties_.subgroup_properties_);

    // Retrieve relevant extension properties.
    if (extensions_.shader_float_controls) {
        SetNext(next, properties_.float_controls_);
    }
    if (extensions_.push_descriptor) {
        SetNext(next, properties_.push_descriptor_);
    }
    if (extensions_.subgroup_size_control || features_.subgroup_size_control.subgroupSizeControl) {
        SetNext(next, properties_.subgroup_size_control_);
    }
    if (extensions_.transform_feedback) {
        SetNext(next, properties_.transform_feedback_);
    }
    properties2_ = physical_.getProperties2();
    properties_.properties_ = properties2_.properties;
    removeUnsuitableExtensions();
    // Check limits.
    struct Limit {
            uint32_t minimum;
            uint32_t value;
            const char* name;
    };

    const vk::PhysicalDeviceLimits& limits{properties_.properties_.limits};
    const std::array limits_report{
        Limit{65536, limits.maxUniformBufferRange, "maxUniformBufferRange"},
        Limit{16, limits.maxViewports, "maxViewports"},
        Limit{8, limits.maxColorAttachments, "maxColorAttachments"},
        Limit{8, limits.maxClipDistances, "maxClipDistances"},
    };

    for (const auto& [min, value, name] : limits_report) {
        if (value < min) {
            SPDLOG_ERROR("{} has to be {} or greater but it is {}", name, min, value);
            suitable = false;
        }
    }
    return suitable;
}
void Device::removeUnsuitableExtensions() {
    // VK_EXT_custom_border_color
    extensions_.custom_border_color = features_.custom_border_color.customBorderColors &&
                                      features_.custom_border_color.customBorderColorWithoutFormat;
    removeExtensionFeatureIfUnsuitable(extensions_.custom_border_color,
                                       features_.custom_border_color,
                                       VK_EXT_CUSTOM_BORDER_COLOR_EXTENSION_NAME);

    // VK_EXT_depth_bias_control
    extensions_.depth_bias_control =
        features_.depth_bias_control.depthBiasControl &&
        features_.depth_bias_control.leastRepresentableValueForceUnormRepresentation;
    removeExtensionFeatureIfUnsuitable(extensions_.depth_bias_control, features_.depth_bias_control,
                                       VK_EXT_DEPTH_BIAS_CONTROL_EXTENSION_NAME);

    // VK_EXT_depth_clip_control
    extensions_.depth_clip_control =
        static_cast<bool>(features_.depth_clip_control.depthClipControl);
    removeExtensionFeatureIfUnsuitable(extensions_.depth_clip_control, features_.depth_clip_control,
                                       VK_EXT_DEPTH_CLIP_CONTROL_EXTENSION_NAME);

    // VK_EXT_extended_dynamic_state
    extensions_.extended_dynamic_state =
        static_cast<bool>(features_.extended_dynamic_state.extendedDynamicState);
    removeExtensionFeatureIfUnsuitable(extensions_.extended_dynamic_state,
                                       features_.extended_dynamic_state,
                                       VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME);

    // VK_EXT_extended_dynamic_state2
    extensions_.extended_dynamic_state2 =
        static_cast<bool>(features_.extended_dynamic_state2.extendedDynamicState2);
    removeExtensionFeatureIfUnsuitable(extensions_.extended_dynamic_state2,
                                       features_.extended_dynamic_state2,
                                       VK_EXT_EXTENDED_DYNAMIC_STATE_2_EXTENSION_NAME);

    // VK_EXT_extended_dynamic_state3
    misc_features_.dynamic_state3_blending =
        features_.extended_dynamic_state3.extendedDynamicState3ColorBlendEnable &&
        features_.extended_dynamic_state3.extendedDynamicState3ColorBlendEquation &&
        features_.extended_dynamic_state3.extendedDynamicState3ColorWriteMask;
    misc_features_.dynamic_state3_enables =
        features_.extended_dynamic_state3.extendedDynamicState3DepthClampEnable &&
        features_.extended_dynamic_state3.extendedDynamicState3LogicOpEnable;

    extensions_.extended_dynamic_state3 =
        misc_features_.dynamic_state3_blending || misc_features_.dynamic_state3_enables;
    misc_features_.dynamic_state3_blending =
        misc_features_.dynamic_state3_blending && extensions_.extended_dynamic_state3;
    misc_features_.dynamic_state3_enables =
        misc_features_.dynamic_state3_enables && extensions_.extended_dynamic_state3;
    removeExtensionFeatureIfUnsuitable(extensions_.extended_dynamic_state3,
                                       features_.extended_dynamic_state3,
                                       VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME);

    // VK_EXT_provoking_vertex
    extensions_.provoking_vertex =
        features_.provoking_vertex.provokingVertexLast &&
        features_.provoking_vertex.transformFeedbackPreservesProvokingVertex;
    removeExtensionFeatureIfUnsuitable(extensions_.provoking_vertex, features_.provoking_vertex,
                                       VK_EXT_PROVOKING_VERTEX_EXTENSION_NAME);

    // VK_KHR_shader_atomic_int64
    extensions_.shader_atomic_int64 = features_.shader_atomic_int64.shaderBufferInt64Atomics &&
                                      features_.shader_atomic_int64.shaderSharedInt64Atomics;
    removeExtensionFeatureIfUnsuitable(extensions_.shader_atomic_int64,
                                       features_.shader_atomic_int64,
                                       VK_KHR_SHADER_ATOMIC_INT64_EXTENSION_NAME);

    // VK_EXT_shader_demote_to_helper_invocation
    extensions_.shader_demote_to_helper_invocation = static_cast<bool>(
        features_.shader_demote_to_helper_invocation.shaderDemoteToHelperInvocation);
    removeExtensionFeatureIfUnsuitable(extensions_.shader_demote_to_helper_invocation,
                                       features_.shader_demote_to_helper_invocation,
                                       VK_EXT_SHADER_DEMOTE_TO_HELPER_INVOCATION_EXTENSION_NAME);

    // VK_EXT_subgroup_size_control
    extensions_.subgroup_size_control =
        features_.subgroup_size_control.subgroupSizeControl &&
        properties_.subgroup_size_control_.minSubgroupSize <= GUEST_WARP_SIZE &&
        properties_.subgroup_size_control_.maxSubgroupSize >= GUEST_WARP_SIZE;
    removeExtensionFeatureIfUnsuitable(extensions_.subgroup_size_control,
                                       features_.subgroup_size_control,
                                       VK_EXT_SUBGROUP_SIZE_CONTROL_EXTENSION_NAME);

    // VK_EXT_transform_feedback
    extensions_.transform_feedback =
        features_.transform_feedback.transformFeedback &&
        features_.transform_feedback.geometryStreams &&
        properties_.transform_feedback_.maxTransformFeedbackStreams >= 4 &&
        properties_.transform_feedback_.maxTransformFeedbackBuffers > 0 &&
        properties_.transform_feedback_.transformFeedbackQueries &&
        properties_.transform_feedback_.transformFeedbackDraw;
    removeExtensionFeatureIfUnsuitable(extensions_.transform_feedback, features_.transform_feedback,
                                       VK_EXT_TRANSFORM_FEEDBACK_EXTENSION_NAME);

    // VK_EXT_vertex_input_dynamic_state
    extensions_.vertex_input_dynamic_state =
        static_cast<bool>(features_.vertex_input_dynamic_state.vertexInputDynamicState);
    removeExtensionFeatureIfUnsuitable(extensions_.vertex_input_dynamic_state,
                                       features_.vertex_input_dynamic_state,
                                       VK_EXT_VERTEX_INPUT_DYNAMIC_STATE_EXTENSION_NAME);

    // VK_KHR_pipeline_executable_properties
    auto vulkan = common::settings::get<settings::RenderVulkan>();
    if (vulkan.renderer_shader_feedback) {
        extensions_.pipeline_executable_properties =
            static_cast<bool>(features_.pipeline_executable_properties.pipelineExecutableInfo);
        removeExtensionFeatureIfUnsuitable(extensions_.pipeline_executable_properties,
                                           features_.pipeline_executable_properties,
                                           VK_KHR_PIPELINE_EXECUTABLE_PROPERTIES_EXTENSION_NAME);
    } else {
        removeExtensionFeature(extensions_.pipeline_executable_properties,
                               features_.pipeline_executable_properties,
                               VK_KHR_PIPELINE_EXECUTABLE_PROPERTIES_EXTENSION_NAME);
    }

    // VK_KHR_workgroup_memory_explicit_layout
    extensions_.workgroup_memory_explicit_layout =
        features_.features.shaderInt16 &&
        features_.workgroup_memory_explicit_layout.workgroupMemoryExplicitLayout &&
        features_.workgroup_memory_explicit_layout.workgroupMemoryExplicitLayout8BitAccess &&
        features_.workgroup_memory_explicit_layout.workgroupMemoryExplicitLayout16BitAccess &&
        features_.workgroup_memory_explicit_layout.workgroupMemoryExplicitLayoutScalarBlockLayout;
    removeExtensionFeatureIfUnsuitable(extensions_.workgroup_memory_explicit_layout,
                                       features_.workgroup_memory_explicit_layout,
                                       VK_KHR_WORKGROUP_MEMORY_EXPLICIT_LAYOUT_EXTENSION_NAME);
}
template <typename Feature>
void Device::removeExtensionFeatureIfUnsuitable(bool is_suitable, Feature& feature,
                                                const std::string& extension_name) {
    if (loaded_extensions_.contains(extension_name) && !is_suitable) {
        SPDLOG_WARN("Removing features for unsuitable extension {}", extension_name);
        this->removeExtensionFeature(is_suitable, feature, extension_name);
    }
}
template <typename Feature>
void Device::removeExtensionFeature(bool& extension, Feature& feature,
                                    const std::string& extension_name) {
    // Unload extension.
    this->removeExtension(extension, extension_name);

    // Save sType and pNext for chain.
    VkStructureType sType = feature.sType;
    void* pNext = feature.pNext;
    auto* current = static_cast<VkBaseOutStructure*>(features2_.pNext);
    // Clear feature struct and restore chain.
    while (current) {
        if (current->pNext->sType == sType) {
            current->pNext = static_cast<VkBaseOutStructure*>(pNext);
            break;
        }

        current = current->pNext;
    }
}

void Device::removeExtension(bool& extension, const std::string& extension_name) {
    extension = false;
    loaded_extensions_.erase(extension_name);
}

void Device::setupFamilies(vk::SurfaceKHR surface) {
    auto queue_family_properties = physical_.getQueueFamilyProperties();
    std::optional<uint32_t> graphics;
    std::optional<uint32_t> present;
    std::optional<uint32_t> compute;
    for (uint32_t index = 0; auto& queue_family : queue_family_properties) {
        if (graphics && (present || !surface) && compute) {
            break;
        }
        if (queue_family.queueCount == 0) {
            continue;
        }
        if ((queue_family.queueFlags & ::vk::QueueFlagBits::eCompute)) {
            compute = index;
        }

        if (queue_family.queueFlags & ::vk::QueueFlagBits::eGraphics) {
            graphics = index;
        }
        if (surface && physical_.getSurfaceSupportKHR(index, surface)) {
            present = index;
        }
        index++;
    }
    if (!graphics) {
        SPDLOG_ERROR("render vulkan Device lacks a graphics queue");
        throw std::runtime_error("vulkan Device lacks a graphics queue");
    }
    if (!compute) {
        SPDLOG_ERROR("render vulkan Device lacks a compute queue");
        throw std::runtime_error("vulkan Device lacks a compute queue");
    }
    if (surface && !present) {
        SPDLOG_ERROR("render Device lacks a present queue");
        throw std::runtime_error("vulkan Device lacks a present queue");
    }
    if (graphics) {
        graphics_family_ = *graphics;
    }
    if (present) {
        present_family_ = *present;
    }
    if (compute) {
        compute_family_ = *compute;
    }
}

auto Device::getDeviceQueueCreateInfos() const -> std::vector<vk::DeviceQueueCreateInfo> {
    static constexpr float QUEUE_PRIORITY = 1.0f;

    std::unordered_set<uint32_t> unique_queue_families{graphics_family_, present_family_,
                                                       compute_family_};
    std::vector<vk::DeviceQueueCreateInfo> queue_cis;
    queue_cis.reserve(unique_queue_families.size());

    for (const uint32_t queue_family : unique_queue_families) {
        ::vk::DeviceQueueCreateInfo queueCreateInfo(::vk::DeviceQueueCreateFlags(), queue_family, 1,
                                                    &QUEUE_PRIORITY);
        queue_cis.emplace_back(queueCreateInfo);
    }

    return queue_cis;
}

auto Device::testDepthStencilBlits(vk::Format format) const -> bool {
    static constexpr vk::FormatFeatureFlags required_features =
        vk::FormatFeatureFlagBits::eBlitSrc | vk::FormatFeatureFlagBits::eBlitDst;
    const auto test_features = [](vk::FormatProperties props) {
        return (props.optimalTilingFeatures & required_features) == required_features;
    };
    return test_features(format_properties_.at(format));
}

auto Device::computeIsOptimalAstcSupported() const -> bool {
    // Disable for now to avoid converting ASTC twice.
    static constexpr std::array astc_formats = {vk::Format::eAstc4x4UnormBlock,
                                                vk::Format::eAstc4x4SrgbBlock,
                                                vk::Format::eAstc5x4UnormBlock,
                                                vk::Format::eAstc5x4SrgbBlock,
                                                vk::Format::eAstc5x5UnormBlock,
                                                vk::Format::eAstc5x5SrgbBlock,
                                                vk::Format::eAstc6x5UnormBlock,
                                                vk::Format::eAstc6x5SrgbBlock,
                                                vk::Format::eAstc6x6UnormBlock,
                                                vk::Format::eAstc6x6SrgbBlock,
                                                vk::Format::eAstc8x5UnormBlock,
                                                vk::Format::eAstc8x5SrgbBlock,
                                                vk::Format::eAstc8x6UnormBlock,
                                                vk::Format::eAstc8x6SrgbBlock,
                                                vk::Format::eAstc8x8UnormBlock,
                                                vk::Format::eAstc8x8SrgbBlock,
                                                vk::Format::eAstc10x5UnormBlock,
                                                vk::Format::eAstc10x5SrgbBlock,
                                                vk::Format::eAstc10x6UnormBlock,
                                                vk::Format::eAstc10x6SrgbBlock,
                                                vk::Format::eAstc10x8UnormBlock,
                                                vk::Format::eAstc10x8SrgbBlock,
                                                vk::Format::eAstc10x10UnormBlock,
                                                vk::Format::eAstc10x10SrgbBlock,
                                                vk::Format::eAstc12x10UnormBlock,
                                                vk::Format::eAstc12x10SrgbBlock,
                                                vk::Format::eAstc12x12UnormBlock,
                                                vk::Format::eAstc12x12SrgbBlock

    };
    if (!features_.features.textureCompressionASTC_LDR) {
        return false;
    }
    const auto format_feature_usage{
        vk::FormatFeatureFlagBits::eSampledImage | vk::FormatFeatureFlagBits::eBlitSrc |
        vk::FormatFeatureFlagBits::eBlitDst | vk::FormatFeatureFlagBits::eTransferSrc |
        vk::FormatFeatureFlagBits::eTransferDst};
    return std::ranges::all_of(astc_formats, [&](const auto format) {
        const auto physical_format_properties{physical_.getFormatProperties(format)};
        return (physical_format_properties.optimalTilingFeatures & format_feature_usage) ==
               format_feature_usage;
    });
    return true;
}

auto Device::isFormatSupported(vk::Format wanted_format, vk::FormatFeatureFlags wanted_usage,
                               utils::FormatType format_type) const -> bool {
    const auto it = format_properties_.find(wanted_format);
    if (it == format_properties_.end()) {
        SPDLOG_ERROR("Unimplemented format");
        return true;
    }
    const auto supported_usage = getFormatFeatures(it->second, format_type);
    return (supported_usage & wanted_usage) == wanted_usage;
}

void Device::collectPhysicalMemoryInfo() {
    using namespace common::literals;
    // Calculate limits using memory budget
    vk::PhysicalDeviceMemoryBudgetPropertiesEXT budget{};
    // 创建 vk::PhysicalDeviceMemoryProperties2 结构体
    vk::PhysicalDeviceMemoryProperties2 memoryProperties2;

    // 创建 vk::PhysicalDeviceMemoryBudgetPropertiesEXT 结构体
    vk::PhysicalDeviceMemoryBudgetPropertiesEXT memoryBudgetProperties;

    // 将 vk::PhysicalDeviceMemoryBudgetPropertiesEXT 结构体链接到
    // vk::PhysicalDeviceMemoryProperties2
    memoryProperties2.pNext = &memoryBudgetProperties;
    physical_.getMemoryProperties2(&memoryProperties2);
    const auto& mem_properties = memoryProperties2.memoryProperties;
    device_access_memory_ = 0;
    uint64_t device_initial_usage = 0;
    uint64_t local_memory = 0;
    for (size_t element = 0; auto memory_heaps : mem_properties.memoryHeaps) {
        const bool is_heap_local = (memory_heaps.flags & vk::MemoryHeapFlagBits::eDeviceLocal) ==
                                   vk::MemoryHeapFlagBits::eDeviceLocal;
        if (!misc_features_.is_integrated && !is_heap_local) {
            continue;
        }
        valid_heap_memory_.push_back(element);
        if (is_heap_local) {
            local_memory += memory_heaps.size;
        }
        if (extensions_.memory_budget) {
            device_initial_usage += budget.heapUsage[element];
            device_access_memory_ += budget.heapBudget[element];
            continue;
        }
        device_access_memory_ += memory_heaps.size;
        element++;
    }
    if (!misc_features_.is_integrated) {
        const uint64_t reserve_memory = std::min<uint64_t>(device_access_memory_ / 8, 1_GiB);
        device_access_memory_ -= reserve_memory;
        auto renderVulkan = common::settings::get<settings::RenderVulkan>();
        auto resolutionScalingInfo = common::settings::get<settings::ResolutionScalingInfo>();
        if (renderVulkan.v_ram_usage_mode != settings::enums::VramUsageMode::Aggressive) {
            // Account for resolution scaling in memory limits
            const size_t normal_memory = 6_GiB;
            const size_t scaler_memory = 1_GiB * resolutionScalingInfo.ScaleUp(1);
            device_access_memory_ =
                std::min<uint64_t>(device_access_memory_, normal_memory + scaler_memory);
        }

        return;
    }
    const auto available_memory =
        static_cast<std::int64_t>(device_access_memory_ - device_initial_usage);
    device_access_memory_ = static_cast<uint64_t>(
        std::max<std::int64_t>(std::min<std::int64_t>(available_memory - 8_GiB, 4_GiB),
                               std::min<std::int64_t>(local_memory, 4_GiB)));
}
void Device::collectToolingInfo() {
    if (!extensions_.tooling_info) {
        return;
    }
    auto tools = physical_.getToolProperties();
    for (const vk::PhysicalDeviceToolProperties& tool : tools) {
        const std::string_view name = tool.name;
        SPDLOG_INFO("Attached debugging tool: {}", name);
        misc_features_.has_renderdoc = misc_features_.has_renderdoc || name == "RenderDoc";
        misc_features_.has_nsight_graphics =
            misc_features_.has_nsight_graphics || name == "NVIDIA Nsight Graphics";
        misc_features_.has_radeon_gpu_profiler =
            misc_features_.has_radeon_gpu_profiler || name == "Radeon GPU Profiler";
    }
}

auto Device::hasTimelineSemaphore() const -> bool {
    if (getDriverID() == vk::DriverId::eQualcommProprietary ||
        getDriverID() == vk::DriverId::eMesaTurnip) {
        // Timeline semaphores do not work properly on all Qualcomm drivers.
        // They generally work properly with Turnip drivers, but are problematic on some devices
        // (e.g. ZTE handsets with Snapdragon 870).
        return false;
    }
    return static_cast<bool>(features_.timeline_semaphore.timelineSemaphore);
}

DeviceMemory Device::tryAllocateMemory(const VkMemoryAllocateInfo& ai) const noexcept {
    VkDeviceMemory memory;
    memory = getLogical().allocateMemory(ai);
    return DeviceMemory(memory, getLogical());
}

}  // namespace render::vulkan
