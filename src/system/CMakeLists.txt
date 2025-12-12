
set(LIB_NAME system)
set(sources
    camera_system.hpp
    logger_system.cpp
    setting_ui.cpp
    pick_system.hpp
    pick_system.cpp
    embree_picker.hpp
    embree_picker.cpp
    transform_system.hpp
    transform_system.cpp
)

set(HEADER_FILES
    logger_system.hpp # 头文件
    setting_ui.hpp
)

set(MODULE_FILES
    setting.cppm # 模块接口文件
)

add_library(${LIB_NAME} STATIC)

# 添加模块文件 (CXX_MODULES) - 这是解决你错误的关键！
# target_sources(${LIB_NAME} PUBLIC
#     FILE_SET CXX_MODULES
#     FILES ${MODULE_FILES} # 这里传入 setting.cppm
# )

# 添加头文件 (HEADERS) - 可选，主要用于 IDE 显示
target_sources(${LIB_NAME} PUBLIC
    FILE_SET HEADERS
    FILES ${HEADER_FILES}
)

# 添加普通源文件 (SOURCES)
target_sources(${LIB_NAME} PRIVATE ${sources})

if(MSVC)
    target_compile_options(${LIB_NAME} PRIVATE
        /we4242 # 'identifier': conversion from 'type1' to 'type2', possible loss of data
        /we4244 # 'conversion': conversion from 'type1' to 'type2', possible loss of data
        /we4245 # 'conversion': conversion from 'type1' to 'type2', signed/unsigned mismatch
        /we4254 # 'operator': conversion from 'type1:field_bits' to 'type2:field_bits', possible loss of data
        /we4800 # Implicit conversion from 'type' to bool. Possible information loss
    )

# target_compile_features(system
# PUBLIC
# cxx_std_23
# )
else()
    target_compile_options(${LIB_NAME} PRIVATE
        -Werror=conversion

        -Wno-sign-conversion
        -Wno-cast-function-type

        $<$<CXX_COMPILER_ID:Clang>:-fsized-deallocation>
    )
endif()

if(TARGET abseil::abseil)
    target_link_libraries(${LIB_NAME} PUBLIC abseil::abseil)
else()
    target_link_libraries(${LIB_NAME} PUBLIC absl::strings)
endif()

if(TARGET embree::embree)
    target_link_libraries(${LIB_NAME} PUBLIC embree::embree)
else()
    target_link_libraries(${LIB_NAME} PUBLIC embree)
endif()

target_link_libraries(${LIB_NAME} PRIVATE spdlog::spdlog Imgui::Imgui ui utils common glm::glm)

# 设置动态库/静态库生成路径
set(LIBRARY_OUTPUT_PATH ${LIB_NAME}/lib)
set_target_properties(${LIB_NAME} PROPERTIES VERSION 0.0.1 SOVERSION 0)
create_target_directory_groups(${LIB_NAME})