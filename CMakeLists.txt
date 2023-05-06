cmake_minimum_required(VERSION 3.19)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(graphics)

set(EXECUTE_NAME graphics)
set(CMAKE_CXX_STANDARD 23)
set(BUILD_TESTING ON)
aux_source_directory(./src MAIN_SRC)
aux_source_directory(./src/resource RESOURCE_SRC)
aux_source_directory(./src/core CORE_SRC)
aux_source_directory(./src/system SYSTEM_SRC)
aux_source_directory(./src/imgui IMGUI)
set(SRC ${MAIN_SRC} ${RESOURCE_SRC} ${CORE_SRC} ${SYSTEM_SRC} ${IMGUI})
add_executable(${EXECUTE_NAME} ${SRC})

find_program(GLSLC_PROGRAM glslc REQUIRED)
message(STATUS "run glslc to compile shaders ...")
execute_process(COMMAND ${GLSLC_PROGRAM} ${CMAKE_SOURCE_DIR}/shader/simple_shader.vert -o ${CMAKE_SOURCE_DIR}/shader/vert.spv)
execute_process(COMMAND ${GLSLC_PROGRAM} ${CMAKE_SOURCE_DIR}/shader/simple_shader.frag -o ${CMAKE_SOURCE_DIR}/shader/frag.spv)
message(STATUS "compile shader OK")
#need vcpkg start
find_package(Stb REQUIRED)
find_package(tinyobjloader CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm REQUIRED)
#need vcpkg end

find_package(Vulkan REQUIRED)


if(WIN32)

elseif(UNIX AND APPLE)   
    add_definitions("-DVK_USE_PLATFORM_MACOS_MVK" "-DGL_SILENCE_DEPRECATION")
endif()

# fmt::fmt-header-only spdlog::spdlog_header_only使用mingw编译时需要修改为header-only
target_link_libraries(${EXECUTE_NAME} PRIVATE Vulkan::Vulkan glfw spdlog::spdlog)


set(CPACK_PACKAGE_DESCRIPTION_FILE "${PROJECT_SOURCE_DIR}/Readme.md")
set(CPACK_GENERATOR "ZIP;TGZ")
file(GLOB SHADER_FILE
        "shader/*.spv"
        )
file(COPY ${SHADER_FILE} DESTINATION shader)
file(COPY images DESTINATION .)
file(COPY models DESTINATION .)
install(DIRECTORY "shader/"
        DESTINATION "shader"
        FILES_MATCHING PATTERN "*.spv" )

set(CMAKE_VERBOSE_MAKEFILE ON)

enable_testing()
add_subdirectory(src/test)