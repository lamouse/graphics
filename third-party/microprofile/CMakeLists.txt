cmake_minimum_required(VERSION 3.12)
project(microprofile LANGUAGES C CXX ASM VERSION 4.0.0)

# Determine if we're built as a subproject (using add_subdirectory)
# or if this is the master project.
set(MASTER_PROJECT OFF)

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(MASTER_PROJECT ON)
endif()

# Default to a Release build
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
    message(STATUS "Defaulting to a Release build")
endif()

# Set hard requirements for C++
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
find_package(Stb REQUIRED)
set(MICROPROFILE_SRC
microprofile.h
microprofile.cpp
microprofile_html.h
)
add_library(microprofile STATIC ${MICROPROFILE_SRC})
target_include_directories(microprofile PRIVATE ${Stb_INCLUDE_DIR})

if (WIN32)
    target_link_libraries(microprofile ws2_32)
endif()
if(MSVC)
    target_compile_options(microprofile PRIVATE
        /MP
        /Zi
        /Zo
        /EHsc
    )

    if(${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
        list(APPEND DYNARMIC_CXX_FLAGS
            -Qunused-arguments
            -Wno-missing-braces)
    endif()
else()
    set(DYNARMIC_CXX_FLAGS
        -Wall
        -Wextra
        -Wcast-qual
        -pedantic
        -Wno-missing-braces)

    if(DYNARMIC_WARNINGS_AS_ERRORS)
        list(APPEND DYNARMIC_CXX_FLAGS
            -pedantic-errors
            -Werror)
    endif()

    if(DYNARMIC_FATAL_ERRORS)
        list(APPEND DYNARMIC_CXX_FLAGS
            -Wfatal-errors)
    endif()

    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        # GCC produces bogus -Warray-bounds warnings from xbyak headers for code paths that are not
        # actually reachable.  Specifically, it happens in cases where some code casts an Operand&
        # to Address& after first checking isMEM(), and that code is inlined in a situation where
        # GCC knows that the variable is actually a Reg64.  isMEM() will never return true for a
        # Reg64, but GCC doesn't know that.
        list(APPEND DYNARMIC_CXX_FLAGS -Wno-array-bounds)
    endif()

    if(CMAKE_CXX_COMPILER_ID MATCHES "[Cc]lang")
        # Bracket depth determines maximum size of a fold expression in Clang since 9c9974c3ccb6.
        # And this in turns limits the size of a std::array.
        list(APPEND DYNARMIC_CXX_FLAGS -fbracket-depth=1024)
    endif()
endif()

set(TARGETS_EXPORT_NAME "${PROJECT_NAME}Targets")
#
# Install
#
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
install(
    TARGETS ${PROJECT_NAME}
    EXPORT "${TARGETS_EXPORT_NAME}"
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(EXPORT "${TARGETS_EXPORT_NAME}"
    FILE "${TARGETS_EXPORT_NAME}.cmake"
    NAMESPACE microprofile::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/microprofile"
)

configure_package_config_file(CMakeModules/microprofileConfig.cmake.in
    ${PROJECT_NAME}Config.cmake
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/microprofile"
)
write_basic_package_version_file(${PROJECT_NAME}ConfigVersion.cmake
    COMPATIBILITY SameMajorVersion
    ARCH_INDEPENDENT
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/microprofile"
)

install(DIRECTORY src/microprofile TYPE INCLUDE FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp")
