# 支持的模型格式
set(MODEL_SOURCE_DIR ${CMAKE_SOURCE_DIR}/models)
set(MODEL_OUTPUT_DIR ${CMAKE_BINARY_DIR}/bin/models)


# 查找所有模型文件
set(MODEL_EXTENSIONS glb gltf obj fbx dae stl ply mtl json)
set(MODEL_FILES "")
foreach(ext IN LISTS MODEL_EXTENSIONS)
    file(GLOB_RECURSE tmp
        LIST_DIRECTORIES false
        RELATIVE "${MODEL_SOURCE_DIR}"
        CONFIGURE_DEPENDS
        "${MODEL_SOURCE_DIR}/*.${ext}"
    )
    list(APPEND MODEL_FILES ${tmp})
endforeach()

set(MODEL_COPY_OUT_PUT "")
foreach(MODEL_FILE IN LISTS MODEL_FILES)
    set(COPY_OUTPUT_PATH "${MODEL_OUTPUT_DIR}/${MODEL_FILE}")
    set(SOURCE_PATH "${MODEL_SOURCE_DIR}/${MODEL_FILE}")
    add_custom_command(
        OUTPUT ${COPY_OUTPUT_PATH}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SOURCE_PATH}" "${COPY_OUTPUT_PATH}"
        DEPENDS "${SOURCE_PATH}"
    )
    list(APPEND MODEL_COPY_OUT_PUT ${COPY_OUTPUT_PATH})
endforeach()

find_program(PYTHON python)

set(OUT_PATH "${CMAKE_BINARY_DIR}/bin/models/assets.json")
add_custom_command(
    COMMAND ${CMAKE_COMMAND} -E make_directory "${MODEL_OUTPUT_DIR}"

    OUTPUT ${OUT_PATH}
    COMMAND ${PYTHON} "${CMAKE_SOURCE_DIR}/models/gen_asset_hashes.py"
    "${OUT_PATH}"
    "${CMAKE_SOURCE_DIR}/models/assets.txt"
    DEPENDS
    "${CMAKE_SOURCE_DIR}/models/assets.txt"
    "${CMAKE_SOURCE_DIR}/models/gen_asset_hashes.py"

    # 你可能需要手动列出所有 asset 文件作为依赖，或用 configure_file 触发
)



add_custom_target(AssetHashManifest
    DEPENDS "${OUT_PATH}" "${MODEL_COPY_OUT_PUT}"
)


set(MOUDLE_PATH_INCLUDE ${CMAKE_CURRENT_BINARY_DIR}/include PARENT_SCOPE)
set(MOUDLE_INCLUDE ${CMAKE_CURRENT_BINARY_DIR}/include)
set(MODEL_HASH_PATH "models/assets.json")
set(MODEL_ROOT_PATH "models/")

# 配置模板
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/model_config.hpp.in"
    "${MOUDLE_INCLUDE}/model_config.hpp"
    @ONLY
)
