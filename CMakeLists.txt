if(WIN32)
        if(CMAKE_C_COMPILER MATCHES "gcc" OR CMAKE_CXX_COMPILER MATCHES ".*/g\\+\\+$")
                message(STATUS "Compiler is MinGW GCC on Windows")
                set(VCPKG_TARGET_TRIPLET "x64-mingw-dynamic" CACHE STRING "target triplet" FORCE)
        endif()
endif()

option(VCPKG_MANIFEST_MODE "")

if(VCPKG_MANIFEST_MODE)
        if(NOT $ENV{VCPKG_ROOT} STREQUAL "")
                if(CMAKE_CXX_COMPILER MATCHES "cl.exe")
                        set(VCPKG_INSTALLED_DIR ${CMAKE_CURRENT_LIST_DIR}/.vcpkg_install/MSVC)
                elseif(CMAKE_CXX_COMPILER MATCHES "clang")
                        set(VCPKG_INSTALLED_DIR ${CMAKE_CURRENT_LIST_DIR}/.vcpkg_install/Clang)
                endif()

                list(APPEND VCPKG_MANIFEST_FEATURES "classic-dependencies")
        endif()
endif()

cmake_minimum_required(VERSION 4.0)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "d0edc3af-4c50-42ea-a356-e2862fe7a444") #开启这个需要设置CMAKE_CXX_MODULE_STD ON
project(graphics)
set(EXECUTE_NAME graphics)
set(PROJECT_VERSION_MAJOR 0)
set(PROJECT_VERSION_MINOR 0)
set(PROJECT_VERSION_PATCH 1)

# set(CMAKE_CXX_MODULE_STD ON)
# set(CMAKE_CXX_SCAN_FOR_MODULES ON)
include(CMakeDependentOption)
CMAKE_DEPENDENT_OPTION(USE_FASTER_LD "Check if a faster linker is available" ON "NOT WIN32" OFF)

if(NOT CMAKE_GENERATOR STREQUAL "Ninja")
        message(FATAL_ERROR "This project requires the Ninja generator. Refers to https://cmake.org/cmake/help/latest/manual/cmake-cxxmodules.7.html#generator-support")
endif()

if(NOT $ENV{VCPKG_ROOT} STREQUAL "")
        if("${CMAKE_TOOLCHAIN_FILE}" STREQUAL "")
                set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
                include("${CMAKE_TOOLCHAIN_FILE}")
        endif()
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(TRACY_ENABLE "Enable Tracy profiling support" ON)
option(TRACY_ON_DEMAND "Tracy on on demand" ON)

# 设置可执行文件输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin CACHE PATH "Executable output path")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin CACHE PATH "Library output path")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/lib CACHE PATH "Single Directory for all static libraries.")
set(nlohmann-json_IMPLICIT_CONVERSIONS OFF)
set(BUILD_TESTING ON)

# need vcpkg start
find_package(assimp CONFIG REQUIRED)
find_package(Boost REQUIRED COMPONENTS container)
find_package(spdlog CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Microsoft.GSL CONFIG REQUIRED)
find_package(glslang CONFIG REQUIRED)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
find_package(Qt6Widgets CONFIG REQUIRED)

find_package(spirv-cross CONFIG QUIET)

if(NOT spirv-cross_FOUND)
        find_package(spirv_cross_core CONFIG REQUIRED)
        find_package(spirv_cross_glsl CONFIG REQUIRED)
        find_package(spirv_cross_cpp CONFIG REQUIRED)
endif()

find_package(SDL3 CONFIG REQUIRED)
find_package(EnTT CONFIG REQUIRED)
find_package(absl CONFIG REQUIRED)
find_package(VulkanHeaders CONFIG)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
find_package(VulkanUtilityLibraries CONFIG REQUIRED)
find_package(xxHash CONFIG REQUIRED)
find_package(Tracy CONFIG REQUIRED)
find_package(embree CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(Ktx CONFIG REQUIRED) # vcpkg install ktx --x-abi-tools-use-exact-versions

# need vcpkg end
add_subdirectory(third-party)

# This function should be passed a list of all files in a target. It will automatically generate
# file groups following the directory hierarchy, so that the layout of the files in IDEs matches the
# one in the filesystem.
function(create_target_directory_groups target_name)
        # Place any files that aren't in the source list in a separate group so that they don't get in
        # the way.
        source_group("Other Files" REGULAR_EXPRESSION ".")

        get_target_property(target_sources "${target_name}" SOURCES)

        foreach(file_name IN LISTS target_sources)
                get_filename_component(dir_name "${file_name}" PATH)

                # Group names use '\' as a separator even though the entire rest of CMake uses '/'...
                string(REPLACE "/" "\\" group_name "${dir_name}")
                source_group("${group_name}" FILES "${file_name}")
        endforeach()
endfunction()

# Adjustments for MSVC + Ninja
if(MSVC AND CMAKE_GENERATOR STREQUAL "Ninja")
        add_compile_options(
                /wd4464 # relative include path contains '..'
                /wd4711 # function 'function' selected for automatic inline expansion
                /wd4820 # 'bytes' bytes padding added after construct 'member_name'
        )
endif()

if(USE_FASTER_LD AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        # We will assume that if the compiler is GCC, it will attempt to use ld.bfd by default.
        # Try to pick a faster linker.
        find_program(LLD lld)
        find_program(MOLD mold)

        if(MOLD AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "12.1")
                message(NOTICE "Selecting mold as linker")
                add_link_options("-fuse-ld=mold")
        elseif(LLD)
                message(NOTICE "Selecting lld as linker")
                add_link_options("-fuse-ld=lld")
        endif()
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/CMakeModules")

include(ClangFormat)
enable_testing()

add_subdirectory(shader)
add_subdirectory(models)
add_subdirectory(images)
add_subdirectory(fonts)
add_subdirectory(src)
add_subdirectory(doc)
