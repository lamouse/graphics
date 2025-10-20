
set(LIB_NAME render-core)

include(${CMAKE_CURRENT_SOURCE_DIR}/render_core.cmake)
add_library(${LIB_NAME} STATIC ${sources})

# add_library(core STATIC ${CORE_SRC})
if(MSVC)
    target_compile_options(${LIB_NAME} PRIVATE

        # /we4244 # 'conversion': conversion from 'type1' to 'type2', possible loss of data
        # /we4245 # 'conversion': conversion from 'type1' to 'type2', signed/unsigned mismatch
        # /we4254 # 'operator': conversion from 'type1:field_bits' to 'type2:field_bits', possible loss of data
        # /we4800 # Implicit conversion from 'type' to bool. Possible information loss
    )
else()
    target_compile_options(${LIB_NAME} PRIVATE
        # -Werror=conversion

        -Wno-sign-conversion
        -Wno-cast-function-type

        $<$<CXX_COMPILER_ID:Clang>:-fsized-deallocation>
    )
endif()

add_subdirectory(host_shaders)

add_dependencies(${LIB_NAME} host_shaders)
target_include_directories(${LIB_NAME} PRIVATE ${HOST_SHADERS_INCLUDE})
target_link_libraries(${LIB_NAME} PRIVATE core common shader Imgui::Imgui)
target_link_libraries(${LIB_NAME} PRIVATE Vulkan::Headers GPUOpen::VulkanMemoryAllocator GOOGLE::farmhash)
target_link_libraries(${LIB_NAME} PRIVATE spdlog::spdlog xxHash::xxhash)

if(TRACY_ENABLE)
    message(STATUS "Tracy support is enabled")
    # 添加 Tracy 相关的编译选项或源文件
    add_definitions(-DTRACY_ENABLE)

    if(TRACY_ON_DEMAND)
        add_definitions(-DTRACY_ON_DEMAND)
    endif()

    target_link_libraries(${LIB_NAME} PRIVATE Tracy::TracyClient)
else()
    message(STATUS "Tracy support is disabled")
endif()

# 设置动态库/静态库生成路径
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)
set_target_properties(${LIB_NAME} PROPERTIES VERSION 0.0.1 SOVERSION 0)
create_target_directory_groups(${LIB_NAME})