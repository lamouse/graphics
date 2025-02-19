aux_source_directory(. RENDER_CORE_SRC)
aux_source_directory(./fs FS_SRC)
add_library(common STATIC ${RENDER_CORE_SRC} ${FS_SRC})
#add_library(core STATIC ${CORE_SRC})
if (MSVC)
    target_compile_options(common PRIVATE
        /we4242 # 'identifier': conversion from 'type1' to 'type2', possible loss of data
        /we4244 # 'conversion': conversion from 'type1' to 'type2', possible loss of data
        /we4245 # 'conversion': conversion from 'type1' to 'type2', signed/unsigned mismatch
        /we4254 # 'operator': conversion from 'type1:field_bits' to 'type2:field_bits', possible loss of data
        /we4800 # Implicit conversion from 'type' to bool. Possible information loss
    )
else()
    target_compile_options(common PRIVATE
        -Werror=conversion

        -Wno-sign-conversion
        -Wno-cast-function-type

        $<$<CXX_COMPILER_ID:Clang>:-fsized-deallocation>
    )
endif()
target_link_libraries(common PUBLIC Vulkan::Vulkan spdlog::spdlog)
# 设置动态库/静态库生成路径
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)
set_target_properties(common PROPERTIES VERSION 0.0.1 SOVERSION 0)
create_target_directory_groups(common)