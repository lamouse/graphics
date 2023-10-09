cmake_minimum_required(VERSION 3.26)

if(WIN32)
        if(CMAKE_C_COMPILER MATCHES "mingw" OR CMAKE_CXX_COMPILER MATCHES "mingw")
                message(STATUS "---Building with mingw---")
                set(VCPKG_TARGET_TRIPLET "x64-mingw-dynamic" CACHE STRING "target triplet" FORCE)
#                set(CMAKE_CXX_FLAGS " ${CMAKE_CXX_FLAGS} -Wall -Wextra -Wconversion -Wnull-dereference -Wuninitialized -Wunused -Wshadow -Wunused-parameter -Wreturn-type -fstack-check -fstack-clash-protection")
        endif()
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(graphics)
message("Building with CMake version: ${CMAKE_VERSION}")

set(EXECUTE_NAME graphics)
set(CMAKE_CXX_STANDARD 23)

if(WIN32)
elseif(UNIX AND APPLE)
        add_definitions("-DVK_USE_PLATFORM_MACOS_MVK" "-DGL_SILENCE_DEPRECATION")
endif()

if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_definitions(-DNO_DEBUG)
endif()


set(BUILD_TESTING ON)
aux_source_directory(./src MAIN_SRC)
aux_source_directory(./src/resource RESOURCE_SRC)
aux_source_directory(./src/core CORE_SRC)
aux_source_directory(./src/system SYSTEM_SRC)
aux_source_directory(./src/imgui IMGUI)
set(SRC ${MAIN_SRC} ${RESOURCE_SRC} ${CORE_SRC} ${SYSTEM_SRC} ${IMGUI})
add_executable(${EXECUTE_NAME} ${SRC})

# need vcpkg start
find_package(Stb REQUIRED)
find_package(tinyobjloader CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm REQUIRED)
find_package(Microsoft.GSL CONFIG REQUIRED)

# need vcpkg end
find_package(Vulkan REQUIRED)

target_link_libraries(${EXECUTE_NAME} PRIVATE Vulkan::Vulkan glfw spdlog::spdlog Microsoft.GSL::GSL)

set(CPACK_PACKAGE_DESCRIPTION_FILE "${PROJECT_SOURCE_DIR}/Readme.md")
set(CPACK_GENERATOR "ZIP;TGZ")

# file(GLOB SHADER_FILE
# "shader/*.spv"
# )

# file(COPY ${SHADER_FILE} DESTINATION shader)
# install(DIRECTORY "shader/"
# DESTINATION "shader"
# FILES_MATCHING PATTERN "*.spv")
file(COPY images DESTINATION .)
file(COPY models DESTINATION .)

find_program(GLSLC_PROGRAM glslc REQUIRED)
message(STATUS "run glslc to compile shaders ...")
file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/shader)
file(GLOB_RECURSE SHADER_SRC_LIST ${CMAKE_SOURCE_DIR}/shader/*)

foreach(shader_file ${SHADER_SRC_LIST})
        # file relative path from src/
        string(REGEX MATCH "shader/.*" relative_path ${shader_file})

        # delete string "src/"
        string(REGEX REPLACE "shader/" "" target_name ${relative_path})

        # rename
        string(REGEX REPLACE "\\.[^\\.]*$" ".spv" target_name ${target_name})
        execute_process(COMMAND ${GLSLC_PROGRAM} ${shader_file} -o ./shader/${target_name})
endforeach()

message(STATUS "compile shader OK")

enable_testing()
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/build-tools")
add_subdirectory(src/test)
add_subdirectory(doc)