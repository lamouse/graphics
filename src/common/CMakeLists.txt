set(LIB_COMMON_NAME common)
include(${CMAKE_CURRENT_SOURCE_DIR}/common.cmake)
add_library(${LIB_COMMON_NAME} STATIC ${sources})

target_sources(${LIB_COMMON_NAME} PUBLIC
        FILE_SET CXX_MODULES
        FILES ${MODULE_FILES} # 这里传入 setting.cppm
)

if(MSVC)
    target_compile_options(${LIB_COMMON_NAME} PRIVATE
        # /we4244 # 'conversion': conversion from 'type1' to 'type2', possible loss of data
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

if(TRACY_ENABLE)
    message(STATUS "Tracy support is enabled")
    # 添加 Tracy 相关的编译选项或源文件
    target_compile_definitions(${LIB_COMMON_NAME} INTERFACE -DTRACY_ENABLE)

    if(TRACY_ON_DEMAND)
        target_compile_definitions(${LIB_COMMON_NAME} INTERFACE -DTRACY_ON_DEMAND)
    endif()

    target_link_libraries(${LIB_COMMON_NAME} INTERFACE Tracy::TracyClient)
endif()

target_link_libraries(${LIB_COMMON_NAME} PUBLIC spdlog::spdlog xxHash::xxhash Imgui::Imgui system)
target_link_libraries(${LIB_COMMON_NAME} PRIVATE assimp::assimp)

# 设置动态库/静态库生成路径
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)
set_target_properties(${LIB_COMMON_NAME} PROPERTIES VERSION 0.0.1 SOVERSION 0)
create_target_directory_groups(${LIB_COMMON_NAME})