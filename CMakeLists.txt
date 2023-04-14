cmake_minimum_required(VERSION 3.19)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(graphics)

set(EXECUTE_NAME graphics)

set(CMAKE_CXX_STANDARD 23)

aux_source_directory(./src MAIN_SRC)
aux_source_directory(./src/resource RESOURCE_SRC)
aux_source_directory(./src/core CORE_SRC)
aux_source_directory(./src/system SYSTEM_SRC)
aux_source_directory(./src/imgui IMGUI)
set(SRC ${MAIN_SRC} ${RESOURCE_SRC} ${CORE_SRC} ${SYSTEM_SRC} ${IMGUI} )
# aux_source_directory(./src/test TEST_SRC)
# set(SRC ${TEST_SRC})
add_executable(${EXECUTE_NAME} ${SRC})

find_program(GLSLC_PROGRAM glslc REQUIRED)
message(STATUS "run glslc to compile shaders ...")
execute_process(COMMAND ${GLSLC_PROGRAM} ${CMAKE_SOURCE_DIR}/shader/simple_shader.vert -o ${CMAKE_SOURCE_DIR}/shader/vert.spv)
execute_process(COMMAND ${GLSLC_PROGRAM} ${CMAKE_SOURCE_DIR}/shader/simple_shader.frag -o ${CMAKE_SOURCE_DIR}/shader/frag.spv)
message(STATUS "compile shader OK")
#need vcpkg start
find_package(Stb REQUIRED)
find_package(tinyobjloader CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
#need vcpkg end

if(WIN32)
        find_package(glfw3 CONFIG REQUIRED)
        find_package(Vulkan REQUIRED)
        find_package(glm REQUIRED)
        find_package(spdlog CONFIG REQUIRED)


        target_link_libraries(${EXECUTE_NAME} PUBLIC glfw Vulkan::Vulkan PRIVATE fmt::fmt-header-only spdlog::spdlog spdlog::spdlog_header_only)
elseif(UNIX AND APPLE)
        set(librariesList
                GLFW
                vulkan
        )
    
    add_definitions("-DVK_USE_PLATFORM_MACOS_MVK" "-DGL_SILENCE_DEPRECATION")
        find_library(${librariesList})
        include_directories("/usr/local/include")
        target_link_directories(${EXECUTE_NAME} PRIVATE
                /usr/local/lib
        )
        target_include_directories(${EXECUTE_NAME} PRIVATE ${Stb_INCLUDE_DIR})
        target_link_libraries(${EXECUTE_NAME} PRIVATE ${librariesList} fmt::fmt-header-only)
endif()


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
