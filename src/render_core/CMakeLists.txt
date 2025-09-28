
set(LIB_NAME render-core)
set(sources
  compatible_formats.cpp
    compatible_formats.h
    delayed_destruction_ring.h
    fixed_pipeline_state.cpp
    fixed_pipeline_state.h
    framebufferConfig.hpp
    framebuffer_config.cpp
    fsr.cpp
    fsr.h
    graphic.hpp
    render_base.cpp
    render_base.hpp
    shader_notify.cpp
    shader_notify.hpp
    surface.cpp
    surface.hpp
    texture.hpp
    uniforms.hpp
    vertex.hpp
    buffer_cache/buffer_base.hpp
    buffer_cache/buffer_cache.h
    buffer_cache/buffer_cache_base.hpp
    buffer_cache/usage_tracker.hpp
    render_vulkan/blit_image.cpp
    render_vulkan/blit_image.hpp
    render_vulkan/blit_screen.cpp
    render_vulkan/blit_screen.hpp
    render_vulkan/buffer_cache.cpp
    render_vulkan/buffer_cache.h
    render_vulkan/command_pool.cpp
    render_vulkan/command_pool.hpp
    render_vulkan/compute_pass.cpp
    render_vulkan/compute_pass.hpp
    render_vulkan/compute_pipeline.cpp
    render_vulkan/compute_pipeline.hpp
    render_vulkan/descriptor_pool.cpp
    render_vulkan/descriptor_pool.hpp
    render_vulkan/format_to_vk.cpp
    render_vulkan/format_to_vk.hpp
    render_vulkan/graphics_pipeline.cpp
    render_vulkan/graphics_pipeline.hpp
    render_vulkan/master_semaphore.cpp
    render_vulkan/master_semaphore.hpp
    render_vulkan/pipeline_cache.cpp
    render_vulkan/pipeline_cache.hpp
    render_vulkan/pipeline_helper.hpp
    render_vulkan/pipeline_statistics.cpp
    render_vulkan/pipeline_statistics.hpp
    render_vulkan/present_manager.cpp
    render_vulkan/present_manager.hpp
    render_vulkan/render_pass.cpp
    render_vulkan/render_pass.hpp
    render_vulkan/render_vulkan.cpp
    render_vulkan/render_vulkan.hpp
    render_vulkan/resource_pool.cpp
    render_vulkan/resource_pool.hpp
    render_vulkan/scheduler.cpp
    render_vulkan/scheduler.hpp
    render_vulkan/staging_buffer_pool.cpp
    render_vulkan/staging_buffer_pool.hpp
    render_vulkan/swapchain.cpp
    render_vulkan/swapchain.hpp
    render_vulkan/texture_cache.cpp
    render_vulkan/texture_cache.hpp
    render_vulkan/update_descriptor.cpp
    render_vulkan/update_descriptor.hpp
    render_vulkan/vk_graphic.cpp
    render_vulkan/vk_graphic.hpp
    render_vulkan/vk_imgui.cpp
    render_vulkan/vk_imgui.hpp
    render_vulkan/vk_shader_util.cpp
    render_vulkan/vk_shader_util.hpp
    render_vulkan/present/anti_alias_pass.h
    render_vulkan/present/filters.cpp
    render_vulkan/present/filters.hpp
    render_vulkan/present/fsr.cpp
    render_vulkan/present/fsr.hpp
    render_vulkan/present/layer.cpp
    render_vulkan/present/layer.hpp
    render_vulkan/present/present_push_constants.h
    render_vulkan/present/vulkan_utils.cpp
    render_vulkan/present/vulkan_utils.hpp
    render_vulkan/present/window_adapt_pass.cpp
    render_vulkan/present/window_adapt_pass.hpp
    texture/accelerated_swizzle.cpp
    texture/accelerated_swizzle.h
    texture/formatter.cpp
    texture/formatter.h
    texture/image_base.cpp
    texture/image_base.hpp
    texture/image_info.hpp
    texture/image_view_base.cpp
    texture/image_view_base.hpp
    texture/image_view_info.cpp
    texture/image_view_info.hpp
    texture/render_targets.h
    texture/samples_helper.h
    texture/types.hpp
    texture/util.cpp
    texture/util.hpp
    texture_cache/texture_cache.h
    texture_cache/texture_cache_base.hpp
    texture_cache/utils.cpp
    texture_cache/utils.hpp
    vulkan_common/debug_callback.cpp
    vulkan_common/debug_callback.hpp
    vulkan_common/device.cpp
    vulkan_common/device.hpp
    vulkan_common/device_utils.cpp
    vulkan_common/device_utils.hpp
    vulkan_common/instance.cpp
    vulkan_common/instance.hpp
    vulkan_common/memory_allocator.cpp
    vulkan_common/memory_allocator.hpp
    vulkan_common/vk_surface.cpp
    vulkan_common/vk_surface.hpp
    vulkan_common/vma.cpp
    vulkan_common/vma.hpp
    vulkan_common/vulkan_common.hpp
    vulkan_common/vulkan_wrapper.cpp
    vulkan_common/vulkan_wrapper.hpp
)

add_library(${LIB_NAME} STATIC ${sources})

# add_library(core STATIC ${CORE_SRC})
if(MSVC)
    target_compile_options(${LIB_NAME} PRIVATE

        # /we4242 # 'identifier': conversion from 'type1' to 'type2', possible loss of data
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
target_include_directories(${LIB_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/third-party/imgui/src)
target_link_libraries(${LIB_NAME} PUBLIC core common shader imgui)
target_link_libraries(${LIB_NAME} PRIVATE Vulkan::Vulkan GPUOpen::VulkanMemoryAllocator GOOGLE::farmhash)
target_link_libraries(${LIB_NAME} PUBLIC spdlog::spdlog)

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