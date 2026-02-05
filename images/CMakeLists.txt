# 找到 toktx（可从 PATH 或指定路径）
find_program(TOKTX_EXECUTABLE ktx REQUIRED
    PATHS ${CMAKE_CURRENT_SOURCE_DIR/tool}
    PATHS ENV PATH
    DOC "Path to toktx executable from KTX-Software"
)

# 定义 cubemap 源目录
set(SKYBOX_SOURCE_DIR "${CMAKE_SOURCE_DIR}/images/cube")
set(SKYBOX_BUILD_DIR "${CMAKE_BINARY_DIR}/bin/images/cube")

# 确保输出目录存在
file(MAKE_DIRECTORY ${SKYBOX_BUILD_DIR})

# 列出所有 skybox 名称（目录名）
file(GLOB SKYBOX_NAMES RELATIVE ${SKYBOX_SOURCE_DIR} ${SKYBOX_SOURCE_DIR}/*)
list(FILTER SKYBOX_NAMES EXCLUDE REGEX "^[.].*") # 排除 .DS_Store 等

# 两套命名方案
set(FACE_SET_POSXYZ posx negx posy negy posz negz)
set(FACE_SET_FRONTBACK right left top bottom front back)

# 为每个 skybox 生成 KTX2 规则
foreach(SKYBOX_NAME ${SKYBOX_NAMES})
    set(SRC_DIR "${SKYBOX_SOURCE_DIR}/${SKYBOX_NAME}")
    set(DST_FILE "${SKYBOX_BUILD_DIR}/${SKYBOX_NAME}.ktx2")

    # === 第一步：确定使用哪套命名 + 文件扩展名 ===
    set(USED_FACE_SET "")
    set(EXT "")

    # 先尝试 posx... 命名
    foreach(FACE ${FACE_SET_POSXYZ})
        if(EXISTS "${SRC_DIR}/${FACE}.hdr")
            set(USED_FACE_SET ${FACE_SET_POSXYZ})
            set(EXT ".hdr")
            break()
        elseif(EXISTS "${SRC_DIR}/${FACE}.jpg")
            set(USED_FACE_SET ${FACE_SET_POSXYZ})
            set(EXT ".jpg")
            break()
        elseif(EXISTS "${SRC_DIR}/${FACE}.png")
            set(USED_FACE_SET ${FACE_SET_POSXYZ})
            set(EXT ".png")
            break()
        endif()
    endforeach()

    # 如果没找到，再尝试 front... 命名
    if(USED_FACE_SET STREQUAL "")
        foreach(FACE ${FACE_SET_FRONTBACK})
            if(EXISTS "${SRC_DIR}/${FACE}.hdr")
                set(USED_FACE_SET ${FACE_SET_FRONTBACK})
                set(EXT ".hdr")
                break()
            elseif(EXISTS "${SRC_DIR}/${FACE}.jpg")
                set(USED_FACE_SET ${FACE_SET_FRONTBACK})
                set(EXT ".jpg")
                break()
            elseif(EXISTS "${SRC_DIR}/${FACE}.png")
                set(USED_FACE_SET ${FACE_SET_FRONTBACK})
                set(EXT ".png")
                break()
            endif()
        endforeach()
    endif()

    # 如果两套都没匹配上，报错跳过
    if(USED_FACE_SET STREQUAL "" OR EXT STREQUAL "")
        message(WARNING "Skybox ${SKYBOX_NAME}: missing or unsupported face files (expected posx/negx/... or front/right/... with .png/.jpg/.hdr)")
        continue()
    endif()

    # === 第二步：构建输入文件列表 ===
    set(INPUT_FILES "")

    foreach(FACE ${USED_FACE_SET})
        list(APPEND INPUT_FILES "${SRC_DIR}/${FACE}${EXT}")
    endforeach()

    # === 第三步：生成 KTX2 ===
    add_custom_command(
        OUTPUT ${DST_FILE}
        COMMAND ${TOKTX_EXECUTABLE}
        create
        --format B8G8R8A8_SRGB
        --cubemap
        --generate-mipmap
        ${INPUT_FILES}
        ${DST_FILE}
        DEPENDS ${INPUT_FILES}
        COMMENT "Generating KTX2 cubemap: ${SKYBOX_NAME} (faces: ${USED_FACE_SET}, ext: ${EXT})"
    )

    add_custom_target(ktx2_${SKYBOX_NAME} ALL DEPENDS ${DST_FILE})
    list(APPEND KTX2_TARGETS ktx2_${SKYBOX_NAME})
endforeach()

set(IMAGES_SOURCE_DIR ${CMAKE_SOURCE_DIR}/images/textures)
set(IMAGES_OUTPUT_DIR ${CMAKE_BINARY_DIR}/bin/images/textures)

set(TEXTURE_EXTENSIONS png jpg)
set(IMAGE_FILES "")

foreach(ext IN LISTS TEXTURE_EXTENSIONS)
    file(GLOB_RECURSE tmp
        LIST_DIRECTORIES false
        RELATIVE "${IMAGES_SOURCE_DIR}"
        CONFIGURE_DEPENDS
        "${IMAGES_SOURCE_DIR}/*.${ext}"
    )
    list(APPEND IMAGE_FILES ${tmp})
endforeach()

set(KTX_OUTPUTS "")

foreach(TEXTURE_FILE IN LISTS IMAGE_FILES)
    get_filename_component(TEXTURE_DIR "${TEXTURE_FILE}" DIRECTORY)
    get_filename_component(TEXTURE_NAME_WE "${TEXTURE_FILE}" NAME_WE)

    # 拼接相对目录 + 新扩展名
    set(TEXTURE_OUT_PATH "${IMAGES_OUTPUT_DIR}/${TEXTURE_DIR}/${TEXTURE_NAME_WE}.ktx2")
    set(INPUT_FILE ${IMAGES_SOURCE_DIR}/${TEXTURE_FILE})
    add_custom_command(
        OUTPUT "${TEXTURE_OUT_PATH}"
        COMMAND ${TOKTX_EXECUTABLE}
        create
        --generate-mipmap
        --format B8G8R8A8_SRGB
        --assign-tf srgb
        ${INPUT_FILE}
        ${TEXTURE_OUT_PATH}
        DEPENDS "${INPUT_FILE}"
        COMMENT "Generating KTX2 texture: ${TEXTURE_OUT_PATH}"
    )
    list(APPEND KTX_OUTPUTS "${TEXTURE_OUT_PATH}")
endforeach()
set(IMAGE_TOOL_DEPEND "")
set(IMAGT_TOOLS "")
if(WIN32)
list(APPEND IMAGE_TOOL_DEPEND ktx.exe ktx.dll)
endif()

foreach(DEPEND IN LISTS IMAGE_TOOL_DEPEND)
    set(DEPEND_OUT_PUT_PATH ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tools)
    set(OUT_PATH ${DEPEND_OUT_PUT_PATH}/${DEPEND})
    add_custom_command(
        OUTPUT ${OUT_PATH}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${DEPEND_OUT_PUT_PATH}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/tool/${DEPEND}"
        ${OUT_PATH}
        COMMENT "copy ktx utils"
    )
    list(APPEND IMAGT_TOOLS "${OUT_PATH}")
endforeach()

# 创建总目标
add_custom_target(skyboxes_ktx2 DEPENDS ${KTX2_TARGETS} ${KTX_OUTPUTS} ${IMAGT_TOOLS})

set(TEXTURE_PATH_INCLUDE ${CMAKE_CURRENT_BINARY_DIR}/include PARENT_SCOPE)
set(TEXTURE_INCLUDE ${CMAKE_CURRENT_BINARY_DIR}/include)
set(TEXTURE_ROOT_PATH "images/textures/")
set(CUBE_MAP_PATH "images/cube/")

# 配置模板
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/image_config.hpp.in"
    "${TEXTURE_INCLUDE}/image_config.hpp"
    @ONLY
)
