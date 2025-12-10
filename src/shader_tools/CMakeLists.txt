set(sources
    shader_compile.hpp
    shader_compile.cpp
    shader_enums.hpp
    shader_info.h
    stage.h
    varying_state.hpp
)

add_library(shader STATIC ${sources})

# add_library(core STATIC ${CORE_SRC})
if(MSVC)
    target_compile_options(shader PRIVATE
        /we4242 # 'identifier': conversion from 'type1' to 'type2', possible loss of data
        /we4244 # 'conversion': conversion from 'type1' to 'type2', possible loss of data
        /we4245 # 'conversion': conversion from 'type1' to 'type2', signed/unsigned mismatch
        /we4254 # 'operator': conversion from 'type1:field_bits' to 'type2:field_bits', possible loss of data
        /we4800 # Implicit conversion from 'type' to bool. Possible information loss
    )
else()
    target_compile_options(shader PRIVATE
        -Werror=conversion

        -Wno-sign-conversion
        -Wno-cast-function-type

        $<$<CXX_COMPILER_ID:Clang>:-fsized-deallocation>
    )
endif()

target_link_libraries(shader PUBLIC Boost::container core)
target_link_libraries(shader PRIVATE spirv-cross-core spirv-cross-cpp spirv-cross-glsl)
target_link_libraries(shader PRIVATE glslang::glslang glslang::glslang-default-resource-limits glslang::SPIRV glslang::SPVRemapper)

# 设置动态库/静态库生成路径
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)
set_target_properties(shader PROPERTIES VERSION 0.0.1 SOVERSION 0)
create_target_directory_groups(shader)