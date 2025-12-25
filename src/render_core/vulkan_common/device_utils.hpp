#pragma once

#include <vulkan/vulkan.hpp>
#include <unordered_map>
#include <set>

namespace render::vulkan::utils {
auto GetFormatProperties(vk::PhysicalDevice physical)
    -> std::unordered_map<vk::Format, vk::FormatProperties>;
enum class NvidiaArchitecture {
    Arch_KeplerOrOlder,
    Arch_Maxwell,
    Arch_Pascal,
    Arch_Volta,
    Arch_Turing,
    Arch_AmpereOrNewer
};
// Misc features
struct MiscFeatures {
        bool is_optimal_astc_supported{};           ///< Support for all guest ASTC formats.
        bool is_blit_depth24_stencil8_supported{};  ///< Support for blitting from and to D24S8.
        bool is_blit_depth32_stencil8_supported{};  ///< Support for blitting from and to D32S8.
        bool is_warp_potentially_bigger{};          ///< Host warp size can be bigger than guest.
        bool is_integrated{};                       ///< Is GPU an iGPU.
        bool is_virtual{};                          ///< Is GPU a virtual GPU.
        bool is_non_gpu{};                     ///< Is SoftwareRasterizer, FPGA, non-GPU device.
        bool has_broken_compute{};             ///< Compute shaders can cause crashes
        bool has_broken_cube_compatibility{};  ///< Has broken cube compatibility bit
        bool has_broken_parallel_compiling{};  ///< Has broken parallel shader compiling.
        bool has_renderdoc{};                  ///< Has RenderDoc attached
        bool has_nsight_graphics{};            ///< Has Nsight Graphics attached
        bool has_radeon_gpu_profiler{};        ///< Has Radeon GPU Profiler attached.
        bool cant_blit_msaa{};                 ///< Does not support MSAA<->MSAA blitting.
        bool must_emulate_scaled_formats{};    ///< Requires scaled vertex format emulation
        bool must_emulate_bgr565{};            ///< Emulates BGR565 by swizzling RGB565 format.
        bool dynamic_state3_blending{};        ///< Has all blending features of dynamic_state3.
        bool dynamic_state3_enables{};         ///< Has all enables features of dynamic_state3.
        bool supports_conditional_barriers{};  ///< Allows barriers in conditional control flow.
};
/// Format usage descriptor.
auto getNvidiaArchitecture(vk::PhysicalDevice physical,
                           const std::set<std::string, std::less<>>& exts) -> NvidiaArchitecture;

[[nodiscard]] auto checkBrokenCompute(vk::DriverId driver_id, uint32_t driver_version) -> bool;

auto extensionListForVulkan(const std::set<std::string, std::less<>>& extensions)
    -> std::vector<const char*>;

}  // namespace render::vulkan::utils