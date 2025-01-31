
if(CMAKE_C_COMPILER MATCHES "ucrt" OR CMAKE_CXX_COMPILER MATCHES "ucrt")
        message(STATUS "---Building with ucrt---")
        set(VCPKG_TARGET_TRIPLET "x64-mingw-dynamic" CACHE STRING "target triplet" FORCE)

        if(NOT $ENV{SYS_VCPKG_ROOT} STREQUAL "")
                if("${CMAKE_TOOLCHAIN_FILE}" STREQUAL "")
                        set(CMAKE_TOOLCHAIN_FILE "$ENV{SYS_VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
                        option(VCPKG_MANIFEST_MODE "")
                        include("${CMAKE_TOOLCHAIN_FILE}")
                else()
                        option(VCPKG_MANIFEST_MODE "")
                        include("${CMAKE_TOOLCHAIN_FILE}")
                endif()
        else()
                if(NOT "$ENV{VCPKG_TOOLCHAIN_FILE}" STREQUAL "")
                        # Disable manifest mode (use vcpkg classic mode) when using a custom vcpkg installation
                        option(VCPKG_MANIFEST_MODE "")
                        include("$ENV{VCPKG_TOOLCHAIN_FILE}")
                endif()

                if(NOT ${CMAKE_TOOLCHAIN_FILE} STREQUAL "")
                        option(VCPKG_MANIFEST_MODE "")
                        include("${CMAKE_TOOLCHAIN_FILE}")
                endif()
        endif()
endif()

cmake_minimum_required(VERSION 3.28)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(graphics)
set(EXECUTE_NAME graphics)
set(PROJECT_VERSION_MAJOR 0)
set(PROJECT_VERSION_MINOR 0)
set(PROJECT_VERSION_PATCH 1)
include(CMakeDependentOption)
CMAKE_DEPENDENT_OPTION(USE_FASTER_LD "Check if a faster linker is available" ON "NOT WIN32" OFF)

if(NOT MINGW)
        if(NOT $ENV{SYS_VCPKG_ROOT} STREQUAL "")
                if("${CMAKE_TOOLCHAIN_FILE}" STREQUAL "")
                        set(CMAKE_TOOLCHAIN_FILE "$ENV{SYS_VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
                        option(VCPKG_MANIFEST_MODE "")
                        include("${CMAKE_TOOLCHAIN_FILE}")
                else()
                        option(VCPKG_MANIFEST_MODE "")
                        include("${CMAKE_TOOLCHAIN_FILE}")
                endif()
        else()
                if(NOT "$ENV{VCPKG_TOOLCHAIN_FILE}" STREQUAL "")
                        # Disable manifest mode (use vcpkg classic mode) when using a custom vcpkg installation
                        option(VCPKG_MANIFEST_MODE "")
                        include("$ENV{VCPKG_TOOLCHAIN_FILE}")
                endif()

                if(NOT ${CMAKE_TOOLCHAIN_FILE} STREQUAL "")
                        option(VCPKG_MANIFEST_MODE "")
                        include("${CMAKE_TOOLCHAIN_FILE}")
                endif()
        endif()
endif()

if(MSVC)
        add_compile_options($<$<COMPILE_LANGUAGE:CXX>:/std:c++latest>)

        # boost still makes use of deprecated result_of.
        add_definitions(-D_HAS_DEPRECATED_RESULT_OF)
else()
        set(CMAKE_CXX_STANDARD 23)
        set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

if(WIN32)
elseif(UNIX AND APPLE)
        add_definitions("-DVK_USE_PLATFORM_MACOS_MVK" "-DGL_SILENCE_DEPRECATION")
endif()

# 设置可执行文件输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin CACHE PATH "Executable output path")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin CACHE PATH "Library output path")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/lib CACHE PATH "Single Directory for all static libraries.")
set(BUILD_TESTING ON)

# need vcpkg start
# find_package(tinyobjloader CONFIG)
find_package(assimp CONFIG REQUIRED)
find_package(Boost REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Microsoft.GSL CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)
find_package(glslang CONFIG REQUIRED)
find_package(spirv_cross_core CONFIG REQUIRED)
find_package(spirv_cross_glsl CONFIG REQUIRED)
find_package(spirv_cross_cpp CONFIG REQUIRED)

# need vcpkg end
find_package(Vulkan REQUIRED)


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

enable_testing()
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/CMakeModules")
include(Resource)
include(Config)

include(ClangFormat)

add_subdirectory(src)
add_subdirectory(doc)
