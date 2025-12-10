
set(LIB_RESOURCE_NAME resource)

set (sources
    id.hpp
    id.cpp
    instance.hpp
    instance.cpp
    texture/image.hpp
    texture/image.cpp
    texture/ktx_image.hpp
    texture/ktx_image.cpp
    obj/mesh.hpp
    obj/model_mesh.hpp
    obj/model_mesh.cpp
    obj/particle.hpp
    obj/particle.cpp
    shader/shader.hpp
    shader/shader.cpp
    mesh_instance.hpp
    resource.hpp
    resource.cpp
)

add_library(${LIB_RESOURCE_NAME} STATIC ${sources})
add_dependencies(${LIB_RESOURCE_NAME} build_shaders)
add_dependencies(${LIB_RESOURCE_NAME} AssetHashManifest)
add_dependencies(${LIB_RESOURCE_NAME} skyboxes_ktx2)
if (MSVC)
    target_compile_options(${LIB_RESOURCE_NAME} PRIVATE
        /we4242 # 'identifier': conversion from 'type1' to 'type2', possible loss of data
        /we4244 # 'conversion': conversion from 'type1' to 'type2', possible loss of data
        /we4245 # 'conversion': conversion from 'type1' to 'type2', signed/unsigned mismatch
        /we4254 # 'operator': conversion from 'type1:field_bits' to 'type2:field_bits', possible loss of data
        /we4800 # Implicit conversion from 'type' to bool. Possible information loss
    )
else()
    target_compile_options(${LIB_RESOURCE_NAME} PRIVATE
        -Werror=conversion

        -Wno-sign-conversion
        -Wno-cast-function-type

        $<$<CXX_COMPILER_ID:Clang>:-fsized-deallocation>
    )
endif()

target_include_directories(${LIB_RESOURCE_NAME} PRIVATE ${SHADER_PATH_INCLUDE})
target_include_directories(${LIB_RESOURCE_NAME} PRIVATE ${MOUDLE_PATH_INCLUDE})
target_include_directories(${LIB_RESOURCE_NAME} PRIVATE ${TEXTURE_PATH_INCLUDE})
target_link_libraries(${LIB_RESOURCE_NAME} PRIVATE spdlog::spdlog glm::glm ecs Imgui::Imgui absl::strings)
target_link_libraries(${LIB_RESOURCE_NAME} PUBLIC nlohmann_json::nlohmann_json KTX::ktx assimp::assimp)

# 设置动态库/静态库生成路径
set(LIBRARY_OUTPUT_PATH ${LIB_RESOURCE_NAME}/lib)
set_target_properties(${LIB_RESOURCE_NAME} PROPERTIES VERSION 0.0.1 SOVERSION 0)
create_target_directory_groups(${LIB_RESOURCE_NAME})