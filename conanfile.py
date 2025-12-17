import os
from conan import ConanFile
from conan.tools.cmake import CMake
from conan.tools.cmake import cmake_layout

class GraphicsConan(ConanFile):
    name = "graphics"
    version = "0.1"
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeToolchain", "CMakeDeps"

    def configure(self):
        if self.settings.compiler == "msvc":
            self.settings.compiler.runtime = "dynamic"  # 统一到 /MD
            self.settings.compiler.cppstd = None        #忽略msvc cppstd

    def requirements(self):
        self.requires("stb/cci.20230920")
        self.requires("assimp/6.0.2")
        self.requires("boost/1.80.0")
        self.requires("glfw/3.4")
        self.requires("glm/1.0.1")
        self.requires("ms-gsl/4.2.0")
        self.requires("spirv-cross/1.4.321.0")
        self.requires("glslang/1.4.313.0")
        self.requires("sdl/3.2.20")
        self.requires("spdlog/1.16.0")
        self.requires("fmt/12.1.0", override = True)
        self.requires("entt/3.15.0")
        self.requires("abseil/20250814.0")
        self.requires("vulkan-headers/1.4.313.0")
        self.requires("vulkan-utility-libraries/1.4.313")
        self.requires("vulkan-memory-allocator/3.3.0")
        self.requires("xxhash/0.8.3")
        self.requires("tracy/0.11.1")
        self.requires("ktx/4.3.2")
        self.requires("qt/6.8.3")
        self.requires("embree/4.4.0")
        self.requires("gtest/1.17.0")

    def build_requirements(self):
        self.tool_requires("cmake/4.2.0")

    def layout(self):
        cmake_layout(self)

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        # 只构建某个目标，比如 system 静态库
        cmake.build(target="all_test")
        # 或者只构建 graphics 可执行文件
        cmake.build(target="graphics")
        cmake.test()