find_program(CLANG_FORMAT clang-format)

if(NOT CLANG_FORMAT)
        message(WARNING "Clang format not found! Disabling the clang format target")
else()
        set(SRCS ${PROJECT_SOURCE_DIR}/src)
        set(CCOMMENT "Running clang format against all cpp files in src")

        if(WIN32)
                add_custom_target(clang-format ALL
                        COMMAND powershell.exe -Command "Get-ChildItem '${SRCS}/*' -Include *.cpp,*.h,*.hpp,*.cc -Recurse | ForEach-Object {& '${CLANG_FORMAT}' -style=file:./.clang-format -i $$_.fullname}"
                        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
                        COMMENT ${CCOMMENT})
        elseif(MINGW)
                add_custom_target(clang-format
                        COMMAND find `cygpath -u ${SRCS}` -iname *.h -iname *.hpp *.cc -o -iname *.cpp | xargs `cygpath -u ${CLANG_FORMAT}` -i
                        COMMENT ${CCOMMENT})
        else()
                add_custom_target(clang-format
                        COMMAND find ${SRCS} -iname *.h -o -iname *.cpp | xargs ${CLANG_FORMAT} -i
                        COMMENT ${CCOMMENT})
        endif()

        unset(SRCS)
        unset(CCOMMENT)
endif()