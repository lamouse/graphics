file(COPY images DESTINATION ${CMAKE_BINARY_DIR}/bin)
file(COPY models DESTINATION ${CMAKE_BINARY_DIR}/bin)
find_program(GLSLC_PROGRAM glslc REQUIRED)
message(STATUS "run glslc to compile shaders ...")
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/bin/shader)
file(GLOB_RECURSE SHADER_SRC_LIST ${CMAKE_SOURCE_DIR}/shader/*)

foreach(shader_file ${SHADER_SRC_LIST})
        # file relative path from src/
        string(REGEX MATCH "shader/.*" relative_path ${shader_file})

        # delete string "src/"
        string(REGEX REPLACE "shader/" "" target_name ${relative_path})

        # rename
        string(REGEX REPLACE "\\.[^\\.]*$" ".spv" target_name ${target_name})
        execute_process(COMMAND ${GLSLC_PROGRAM} ${shader_file} -o ${CMAKE_BINARY_DIR}/bin/shader/${target_name})
endforeach()

message(STATUS "compile shader OK")