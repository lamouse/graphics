set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(PROGRAM_NAME "graphics")

include(${CMAKE_CURRENT_SOURCE_DIR}/graphics.cmake)

add_executable(${PROGRAM_NAME} ${sources})
target_compile_definitions(${PROGRAM_NAME} PRIVATE USE_SDL)
# target_compile_definitions(${PROGRAM_NAME} PRIVATE USE_GLFW)



target_link_libraries(${PROGRAM_NAME} PRIVATE core utils render-core resource)
target_link_libraries(${PROGRAM_NAME} PRIVATE glfw spdlog::spdlog Microsoft.GSL::GSL assimp::assimp)
target_link_libraries(${PROGRAM_NAME} PRIVATE yaml-cpp::yaml-cpp)
target_link_libraries(${PROGRAM_NAME} PRIVATE SDL3::SDL3 world system Imgui::Imgui)

target_link_directories(${PROGRAM_NAME} PRIVATE ${CMAKE_BINARY_DIR}/lib)


# 添加生成的代码目录
include_directories(${CMAKE_BINARY_DIR}/generated)

if(MSVC)
    set_target_properties(${PROGRAM_NAME} PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS")
elseif(MINGW)
    set_target_properties(${PROGRAM_NAME} PROPERTIES LINK_FLAGS_RELEASE "-Wl,--subsystem,windows")
endif()