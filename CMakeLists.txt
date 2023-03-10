cmake_minimum_required(VERSION 3.19)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(graphics)

set(EXECUTE_NAME graphics)

set(CMAKE_CXX_STANDARD 23)

find_package(glfw3 CONFIG REQUIRED)
find_package(Vulkan REQUIRED)

if(WIN32)
        include_directories("D:/Program/vulkanSDK/Include", "D:/Program/vcpkg/installed/x64-windows/include")
endif()

aux_source_directory(./src SRC)
add_executable(${EXECUTE_NAME} ${SRC})

target_link_libraries(${EXECUTE_NAME} PUBLIC glfw Vulkan::Vulkan)

set(CPACK_PACKAGE_DESCRIPTION_FILE "${PROJECT_SOURCE_DIR}/Readme.md")
set(CPACK_GENERATOR "ZIP;TGZ")
file(GLOB SHADER_FILE
        "shader/*.spv"
        )
file(COPY ${SHADER_FILE} DESTINATION shader)
install(DIRECTORY "shader/"
        DESTINATION "shader"
        FILES_MATCHING PATTERN "*.spv" )

set(CMAKE_VERBOSE_MAKEFILE ON)
