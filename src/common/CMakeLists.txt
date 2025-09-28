set(LIB_COMMON_NAME common)

set (sources
    alignment.h
    alignment.hpp
    assert.hpp
    bit_cast.h
    bit_field.hpp
    bit_util.h
    common_funcs.hpp
    common_settings.hpp
    common_types.hpp
    div_ceil.hpp
    literals.hpp
    lru_cache.h
    make_unique_for_overwrite.h
    math_util.h
    polyfill_thread.hpp
    scope_exit.h
    scratch_buffer.h
    settings_enums.hpp
    settings.hpp
    settings.cpp
    slot_vector.hpp
    swap.h
    thread_worker.hpp
    thread.hpp
    thread.cpp
    unique_function.h
)
add_library(${LIB_COMMON_NAME} STATIC ${sources})

if(MSVC)
    target_compile_options(${LIB_COMMON_NAME} PRIVATE
        /we4242 # 'identifier': conversion from 'type1' to 'type2', possible loss of data
        /we4244 # 'conversion': conversion from 'type1' to 'type2', possible loss of data
        /we4245 # 'conversion': conversion from 'type1' to 'type2', signed/unsigned mismatch
        /we4254 # 'operator': conversion from 'type1:field_bits' to 'type2:field_bits', possible loss of data
        /we4800 # Implicit conversion from 'type' to bool. Possible information loss
    )
else()
    target_compile_options(${LIB_COMMON_NAME} PRIVATE
        -Werror=conversion

        -Wno-sign-conversion
        -Wno-cast-function-type

        $<$<CXX_COMPILER_ID:Clang>:-fsized-deallocation>
    )

    if(UNIX AND APPLE)
        # for std::stop_source
        target_compile_options(${LIB_COMMON_NAME} PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-fexperimental-library>)
    endif()
endif()



target_link_libraries(${LIB_COMMON_NAME} PUBLIC Vulkan::Vulkan spdlog::spdlog)

# 设置动态库/静态库生成路径
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)
set_target_properties(${LIB_COMMON_NAME} PROPERTIES VERSION 0.0.1 SOVERSION 0)
create_target_directory_groups(${LIB_COMMON_NAME})