
set(LIB_CORE_NAME core)

set(MODULE_FILES
        # core.cppm # 模块接口文件
)

set (sources
    camera/camera.hpp
    camera/camera.cpp
    frontend/framebuffer_layout.hpp
    frontend/framebuffer_layout.cpp
    frontend/window.hpp
    frontend/window.cpp
    core.hpp
    core.cpp
    frame_info.hpp
)

add_library(${LIB_CORE_NAME} STATIC ${sources})
# 添加模块文件 (CXX_MODULES) - 这是解决你错误的关键！
target_sources(${LIB_CORE_NAME} PUBLIC
        FILE_SET CXX_MODULES
        FILES ${MODULE_FILES} # 这里传入 setting.cppm
)
if (MSVC)
    target_compile_options(${LIB_CORE_NAME} PRIVATE
        /we4244 # 'conversion': conversion from 'type1' to 'type2', possible loss of data
        /we4245 # 'conversion': conversion from 'type1' to 'type2', signed/unsigned mismatch
        /we4254 # 'operator': conversion from 'type1:field_bits' to 'type2:field_bits', possible loss of data
        /we4800 # Implicit conversion from 'type' to bool. Possible information loss
    )
else()
    target_compile_options(${LIB_CORE_NAME} PRIVATE
        -Werror=conversion

        -Wno-sign-conversion
        -Wno-cast-function-type

        $<$<CXX_COMPILER_ID:Clang>:-fsized-deallocation>
    )
endif()

target_link_libraries(${LIB_CORE_NAME} PUBLIC spdlog::spdlog glm::glm resource render-core)
# 设置动态库/静态库生成路径
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)
set_target_properties(${LIB_CORE_NAME} PROPERTIES VERSION 0.0.1 SOVERSION 0)
create_target_directory_groups(${LIB_CORE_NAME})