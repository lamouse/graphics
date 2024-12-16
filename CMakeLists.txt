cmake_minimum_required(VERSION 3.28)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(graphics)
set(EXECUTE_NAME graphics)
set(PROJECT_VERSION_MAJOR 0)
set(PROJECT_VERSION_MINOR 0)
set(PROJECT_VERSION_PATCH 1)

if(NOT MINGW)
        if(NOT $ENV{VCPKG_ROOT} STREQUAL "")
                if("${CMAKE_TOOLCHAIN_FILE}" STREQUAL "")
                        set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
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

if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_definitions(-DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_WARN)
else()
        add_definitions(-DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG)
endif()

# 设置可执行文件输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(BUILD_TESTING ON)

# need vcpkg start
# find_package(tinyobjloader CONFIG)
find_package(assimp CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Microsoft.GSL CONFIG REQUIRED)

# need vcpkg end
find_package(Vulkan REQUIRED)

file(COPY images DESTINATION ./bin)
file(COPY models DESTINATION ./bin)

find_program(GLSLC_PROGRAM glslc REQUIRED)
message(STATUS "run glslc to compile shaders ...")
file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/bin/shader)
file(GLOB_RECURSE SHADER_SRC_LIST ${CMAKE_SOURCE_DIR}/shader/*)

foreach(shader_file ${SHADER_SRC_LIST})
        # file relative path from src/
        string(REGEX MATCH "shader/.*" relative_path ${shader_file})

        # delete string "src/"
        string(REGEX REPLACE "shader/" "" target_name ${relative_path})

        # rename
        string(REGEX REPLACE "\\.[^\\.]*$" ".spv" target_name ${target_name})
        execute_process(COMMAND ${GLSLC_PROGRAM} ${shader_file} -o ./bin/shader/${target_name})
endforeach()

message(STATUS "compile shader OK")

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

enable_testing()
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/build-tools")
add_subdirectory(src)
add_subdirectory(src/test)
add_subdirectory(doc)
