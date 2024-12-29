set(CMAKE_INCLUDE_CURRENT_DIR ON)
aux_source_directory(. MAIN_SRC)
aux_source_directory(../resource RESOURCE_SRC)
aux_source_directory(../system SYSTEM_SRC)
aux_source_directory(../ OTHER_SRC)
aux_source_directory(${CMAKE_BINARY_DIR}/generated/config GENERATED_SRC)
set(SRC ${MAIN_SRC} ${RESOURCE_SRC} ${SYSTEM_SRC} ${GENERATED_SRC} ${OTHER_SRC})
set(PROGRAM_NAME "graphics")

add_executable(${PROGRAM_NAME} ${SRC})

target_link_libraries(${PROGRAM_NAME} PRIVATE core imgui utils render-core)
target_link_libraries(${PROGRAM_NAME} PRIVATE Vulkan::Vulkan glfw spdlog::spdlog Microsoft.GSL::GSL assimp::assimp)
target_link_libraries(${PROGRAM_NAME} PRIVATE yaml-cpp::yaml-cpp)

target_link_directories(${PROGRAM_NAME} PRIVATE ${CMAKE_BINARY_DIR}/lib)
# 添加生成的代码目录
include_directories(${CMAKE_BINARY_DIR}/generated)


target_include_directories(${PROGRAM_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/third-party/imgui/src)
if(MSVC)
    set_target_properties(${PROGRAM_NAME} PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS")
elseif(MINGW)
    set_target_properties(${PROGRAM_NAME} PROPERTIES LINK_FLAGS_RELEASE "-Wl,--subsystem,windows")
endif()