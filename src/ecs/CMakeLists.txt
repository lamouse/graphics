
set(LIB_NAME ecs)

set (sources
    scene/scene.hpp
    scene/scene.cpp
    scene/entity.hpp
    scene/entity.cpp
    components/camera_component.hpp
    components/tag_component.hpp
    components/transform_component.hpp
    components/render_state_component.hpp
    ui/cameraUI.hpp
    ui/tagUI.hpp
    ui/transformUI.hpp
    component.hpp
)
add_library(${LIB_NAME} STATIC ${sources})
if (MSVC)
    target_compile_options(${LIB_NAME} PRIVATE
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
target_link_libraries(${LIB_NAME} PUBLIC spdlog::spdlog EnTT::EnTT ImguiHelper glm::glm)
# 设置动态库/静态库生成路径
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)
set_target_properties(${LIB_NAME} PROPERTIES VERSION 0.0.1 SOVERSION 0)
create_target_directory_groups(${LIB_NAME})