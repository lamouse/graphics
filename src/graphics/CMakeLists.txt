set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(PROGRAM_NAME "graphics")

set(sources
    app.hpp
    app.cpp
    glfw_common.hpp
    glfw_common.cpp
    glfw_window.hpp
    glfw_window.cpp
    SDL_common.hpp
    SDL_common.cpp
    SDL_window.hpp
    SDL_window.cpp
    ui.hpp
    ui.cpp
    main.cpp
)

aux_source_directory(. MAIN_SRC)
aux_source_directory(${CMAKE_BINARY_DIR}/generated/config GENERATED_SRC)
set(SRC ${sources} ${GENERATED_SRC})

add_executable(${PROGRAM_NAME} ${SRC})

target_link_libraries(${PROGRAM_NAME} PRIVATE core utils render-core resource)
target_link_libraries(${PROGRAM_NAME} PRIVATE glfw spdlog::spdlog Microsoft.GSL::GSL assimp::assimp)
target_link_libraries(${PROGRAM_NAME} PRIVATE yaml-cpp::yaml-cpp)
target_link_libraries(${PROGRAM_NAME} PRIVATE SDL3::SDL3)

target_link_directories(${PROGRAM_NAME} PRIVATE ${CMAKE_BINARY_DIR}/lib)

# 添加生成的代码目录
include_directories(${CMAKE_BINARY_DIR}/generated)

target_include_directories(${PROGRAM_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/third-party/imgui/src)

if(MSVC)
    set_target_properties(${PROGRAM_NAME} PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS")
elseif(MINGW)
    set_target_properties(${PROGRAM_NAME} PROPERTIES LINK_FLAGS_RELEASE "-Wl,--subsystem,windows")
endif()