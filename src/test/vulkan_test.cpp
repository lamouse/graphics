// #include "render_core/vulkan_common/vk_instance.hpp"
// #include "render_core/vulkan_common/device.hpp"
// #include "shader_tools/shader_compile.hpp"
// #include "model_vert_spv.h"
// #include <gtest/gtest.h>
// #include <print>
// // Demonstrate some basic assertions.

// TEST(VulkanInstance, CreateInstance) {
// #ifdef _WIN32
//     auto instance =
//         render::vulkan::createInstance(1, core::frontend::WindowSystemType::Windows, true);
// #elif defined(__APPLE__)
//     auto instance =
//         render::vulkan::CreateInstance(1, core::frontend::WindowSystemType::Cocoa, true);
// #endif
// }

// TEST(Shader, ShaderCompile) {
//     shader::Info vertex_info = shader::compile::getShaderInfo(MODEL_VERT_SPV);
//     std::println("vertex into {}", vertex_info.image_buffer_descriptors.size());
//     // shaderCompile.compile("./shaders", "./shaders/build");
// }