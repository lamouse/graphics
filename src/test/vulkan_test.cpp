#include <GLFW/glfw3.h>
#include "render_core/vulkan_common/instance.hpp"
#include "render_core/vulkan_common/device.hpp"
#include "shader_tools/shader_compile.hpp"
#include <gtest/gtest.h>
// Demonstrate some basic assertions.

TEST(VulkanInstance, CreateInstance) {
    vk::Instance instance;
#ifdef _WIN32
    instance = render::vulkan::CreateInstance(1, core::frontend::WindowSystemType::Windows, true);
#elif defined(__APPLE__)
    auto instance =
        render::vulkan::CreateInstance(1, core::frontend::WindowSystemType::Cocoa, true);
#endif
    ASSERT_TRUE(instance);
    instance.destroy();
}

TEST(VulkanDriver, CreateDriver) {
    vk::Instance instance;
#ifdef _WIN32
    instance = render::vulkan::CreateInstance(1, core::frontend::WindowSystemType::Windows, true);
#elif defined(__APPLE__)
    auto instance =
        render::vulkan::CreateInstance(1, core::frontend::WindowSystemType::Cocoa, true);
#endif
    ASSERT_TRUE(instance);
    auto phys = instance.enumeratePhysicalDevices();
    ASSERT_TRUE(phys.size() > 0);
    for (auto &pyh : phys) {
        render::vulkan::Device d(instance, pyh, nullptr);
        ASSERT_TRUE(d.getLogical());
        d.getLogical().destroy();
    }

    instance.destroy();
}

TEST(Shader, ShaderCompile) {
    // shader::compile::ShaderCompile shaderCompile;
    // shaderCompile.compile("./shaders", "./shaders/build");
}