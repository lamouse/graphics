set(sources
    shader_compile.hpp
    shader_compile.cpp
    shader_enums.hpp
    shader_info.h
    stage.h
)

set(MODULE_FILES
    shader.cppm
)

set(LIB_NAME shader)

add_library(${LIB_NAME} STATIC ${sources})

target_sources(${LIB_NAME} PUBLIC
        FILE_SET CXX_MODULES
        FILES ${MODULE_FILES}
)

# add_library(core STATIC ${CORE_SRC})
if(MSVC)
    target_compile_options(${LIB_NAME} PRIVATE
        /we4242 # 'identifier': conversion from 'type1' to 'type2', possible loss of data
        /we4244 # 'conversion': conversion from 'type1' to 'type2', possible loss of data
        /we4245 # 'conversion': conversion from 'type1' to 'type2', signed/unsigned mismatch
        /we4254 # 'operator': conversion from 'type1:field_bits' to 'type2:field_bits', possible loss of data
        /we4800 # Implicit conversion from 'type' to bool. Possible information loss
    )
else()
    target_compile_options(${LIB_NAME} PRIVATE
        -Werror=conversion

        -Wno-sign-conversion
        -Wno-cast-function-type

        $<$<CXX_COMPILER_ID:Clang>:-fsized-deallocation>
    )
endif()

target_link_libraries(${LIB_NAME} PUBLIC Boost::container core)

if(TARGET abseil::abseil)
    target_link_libraries(${LIB_NAME} PUBLIC abseil::abseil)
endif()

if(TARGET spirv-cross::spirv-cross)
    target_link_libraries(${LIB_NAME} PRIVATE spirv-cross::spirv-cross)
else()
    target_link_libraries(${LIB_NAME} PRIVATE spirv-cross-core spirv-cross-cpp spirv-cross-glsl)
endif()

if(TARGET glslang::_glslang-do-not-use)
    target_link_libraries(${LIB_NAME} PRIVATE glslang::_glslang-do-not-use)
else()
    target_link_libraries(${LIB_NAME} PRIVATE glslang::glslang glslang::glslang-default-resource-limits glslang::SPIRV)
endif()

# 设置动态库/静态库生成路径
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)
set_target_properties(${LIB_NAME} PROPERTIES VERSION 0.0.1 SOVERSION 0)
create_target_directory_groups(${LIB_NAME})