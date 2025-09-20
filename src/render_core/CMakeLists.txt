aux_source_directory(. RENDER_CORE_SRC)
aux_source_directory(./vulkan_common VULKAN_COMMON_SRC)
aux_source_directory(./render_vulkan RENDER_VULKAN_SRC)
aux_source_directory(./buffer_cache BUFFER_CACHE_SRC)
aux_source_directory(./texture_cache TEXTURE_CACHE_SRC)
aux_source_directory(./render_vulkan/present PRESENT_SRC)
aux_source_directory(./texture TEXTURE_SRC)
add_library(render-core STATIC ${RENDER_CORE_SRC} ${RENDER_VULKAN_SRC}
    ${VULKAN_COMMON_SRC} ${BUFFER_CACHE_SRC} ${PRESENT_SRC} ${TEXTURE_SRC} ${TEXTURE_CACHE_SRC})

# add_library(core STATIC ${CORE_SRC})
if(MSVC)
    target_compile_options(render-core PRIVATE

        # /we4242 # 'identifier': conversion from 'type1' to 'type2', possible loss of data
        # /we4244 # 'conversion': conversion from 'type1' to 'type2', possible loss of data
        # /we4245 # 'conversion': conversion from 'type1' to 'type2', signed/unsigned mismatch
        # /we4254 # 'operator': conversion from 'type1:field_bits' to 'type2:field_bits', possible loss of data
        # /we4800 # Implicit conversion from 'type' to bool. Possible information loss
    )
else()
    target_compile_options(render-core PRIVATE
        # -Werror=conversion

        -Wno-sign-conversion
        -Wno-cast-function-type

        $<$<CXX_COMPILER_ID:Clang>:-fsized-deallocation>
    )
endif()

add_subdirectory(host_shaders)

add_dependencies(render-core host_shaders)
target_include_directories(render-core PRIVATE ${HOST_SHADERS_INCLUDE})
target_include_directories(render-core PRIVATE ${CMAKE_SOURCE_DIR}/third-party/imgui/src)
target_link_libraries(render-core PUBLIC core common shader imgui)
target_link_libraries(render-core PRIVATE Vulkan::Vulkan GPUOpen::VulkanMemoryAllocator GOOGLE::farmhash)
target_link_libraries(render-core PUBLIC spdlog::spdlog)

if(TRACY_ENABLE)
    message(STATUS "Tracy support is enabled")
    # 添加 Tracy 相关的编译选项或源文件
    add_definitions(-DTRACY_ENABLE)

    if(TRACY_ON_DEMAND)
        add_definitions(-DTRACY_ON_DEMAND)
    endif()

    target_link_libraries(render-core PRIVATE Tracy::TracyClient)
else()
    message(STATUS "Tracy support is disabled")
endif()

# 设置动态库/静态库生成路径
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)
set_target_properties(render-core PROPERTIES VERSION 0.0.1 SOVERSION 0)
create_target_directory_groups(render-core)